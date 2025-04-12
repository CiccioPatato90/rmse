// backfilling.cpp
//
// A simple backfilling scheduler implementation for Batsim.
// This implementation uses a list (jobs) for the pending jobs queue,
// a set for available resources, and maps for running jobs and their allocations.

#include <cstdint>
#include <list>
#include <set>
#include <unordered_map>
#include <string>
#include <cstdio>
#include <iterator>
#include <sstream>
#include <fstream>
#include <batprotocol.hpp>
#include <intervalset.hpp>
#include "batsim_edc.h"

using namespace batprotocol;

struct SchedJob {
    std::string job_id;
    uint8_t nb_hosts;
    uint32_t walltime;  // Added walltime field to track job duration
};

// Global variables for scheduler state
static MessageBuilder *mb = nullptr;
static bool format_binary = true;
static std::list<SchedJob*> *jobs = nullptr;
static std::unordered_map<std::string, SchedJob*> running_jobs;
static std::unordered_map<std::string, std::set<uint32_t>> job_allocations;
static uint32_t platform_nb_hosts = 0;
static std::vector<std::set<uint32_t>> available_res;
static std::ofstream log_file;  // Log file stream
static uint32_t backfill_success_count = 0;
static uint32_t contiguous_backfill_count = 0;
static uint32_t non_contiguous_backfill_count = 0;

// -------------------------
// Initialization function
// -------------------------
extern "C" uint8_t batsim_edc_init(const uint8_t *data, uint32_t size, uint32_t flags) {
    (void)data; // Unused
    (void)size; // Unused

    format_binary = ((flags & BATSIM_EDC_FORMAT_BINARY) != 0);
    if ((flags & (BATSIM_EDC_FORMAT_BINARY | BATSIM_EDC_FORMAT_JSON)) != flags) {
        printf("Unknown flags used, cannot initialize backfilling scheduler.\n");
        return 1;
    }
    
    mb = new MessageBuilder(!format_binary);
    jobs = new std::list<SchedJob*>();
    
    
    return 0;
}

// -------------------------
// Deinitialization function
// -------------------------
extern "C" uint8_t batsim_edc_deinit() {
    delete mb;
    mb = nullptr;
    
    if (jobs != nullptr) {
        for (auto *job : *jobs) {
            delete job;
        }
        delete jobs;
        jobs = nullptr;
    }
    
    // Also clean up any running jobs (if still present)
    for (auto &pair : running_jobs) {
        delete pair.second;
    }
    running_jobs.clear();
    job_allocations.clear();
    available_res.clear();

    
    return 0;
}

// Helper function to ensure available_res has enough time slots
void ensure_time_slot_exists(double time) {
    size_t time_index = static_cast<size_t>(time);
    
    // If the time slot doesn't exist yet, create it and all slots up to it
    while (available_res.size() <= time_index) {
        size_t new_index = available_res.size();
        available_res.push_back(std::set<uint32_t>());
        
        // Add all platform resources to the new time slot
        for (uint32_t i = 0; i < platform_nb_hosts; i++) {
            available_res[new_index].insert(i);
        }
    }
}

// Helper function to execute a job
void execute_job(SchedJob* job, const std::set<uint32_t>& resources) {
    // Validate that we have resources to allocate
    if (resources.empty()) {
        return;
    }
    
    std::string resources_str;
    for (auto it = resources.begin(); it != resources.end(); ++it) {
        if (it != resources.begin()) resources_str += ",";
        resources_str += std::to_string(*it);
    }
    mb->add_execute_job(job->job_id, resources_str);
    jobs->pop_front();
}

// -------------------------
// Decision (scheduling) function
// -------------------------
extern "C" uint8_t batsim_edc_take_decisions(
    const uint8_t *what_happened,
    uint32_t what_happened_size,
    uint8_t **decisions,
    uint32_t *decisions_size)
{
    (void) what_happened_size;
    auto *parsed = deserialize_message(*mb, !format_binary, what_happened);
    mb->clear(parsed->now());
    
    double current_time = parsed->now();
    
    auto nb_events = parsed->events()->size();
    for (unsigned int i = 0; i < nb_events; ++i) {
        auto event = (*parsed->events())[i];
        
        switch (event->event_type()) {
            case fb::Event_BatsimHelloEvent: {
                mb->add_edc_hello("best_cont", "1.0.0");
            } break;
            
            case fb::Event_SimulationBeginsEvent: {
                auto simu_begins = event->event_as_SimulationBeginsEvent();
                platform_nb_hosts = simu_begins->computation_host_number();
                
                // Initialize available resources for time 0 (hosts are numbered from 0 to platform_nb_hosts-1)
                ensure_time_slot_exists(0);
                
            } break;
            
            case fb::Event_JobSubmittedEvent: {
                auto parsed_job = event->event_as_JobSubmittedEvent();
                auto job = new SchedJob();
                job->job_id = parsed_job->job_id()->str();
                job->nb_hosts = parsed_job->job()->resource_request();
                job->walltime = parsed_job->job()->walltime();  // Initialize walltime from the job
                
                // Reject jobs that request more hosts than available on the platform
                if (job->nb_hosts > platform_nb_hosts) {
                    mb->add_reject_job(job->job_id);
                    delete job;
                } else {
                    jobs->push_back(job);
                }
            } break;
            
            case fb::Event_JobCompletedEvent: {
                auto parsed_job = event->event_as_JobCompletedEvent();
                std::string completed_job_id = parsed_job->job_id()->str();
                
                // If the job is still running, free its resources
                if (running_jobs.count(completed_job_id)) {
                    SchedJob* completed_job = running_jobs[completed_job_id];
                    
                    // Free resources for all time slots from current time to the end of the job's walltime
                    for (size_t t = static_cast<size_t>(current_time); t < available_res.size(); ++t) {
                        // Add the resources back to the available set for this time slot
                        for (uint32_t host : job_allocations[completed_job_id]) {
                            available_res[t].insert(host);
                        }
                    }
                    
                    running_jobs.erase(completed_job_id);
                    job_allocations.erase(completed_job_id);
                    delete completed_job;
                    
                    
                }
            } break;
            
            default:
                break;
        }
    }
    
    // -------------------------
    // Scheduling loop with backfilling
    // -------------------------
    size_t time_index = static_cast<size_t>(current_time);
    
    // Print queue state
    int queue_position = 0;
 
    
    while (!jobs->empty()) {
        // Always try to schedule the job at the front of the queue first.
        SchedJob* job = jobs->front();
        
        if (available_res[time_index].size() >= job->nb_hosts) {
            // The front job fits: allocate the first nb_hosts resources available.
            
            // Get the first nb_hosts resources from the available set
            std::set<uint32_t> job_resources;
            auto it = available_res[time_index].begin();
            for (uint8_t i = 0; i < job->nb_hosts; ++i, ++it) {
                job_resources.insert(*it);
            }
            
            // Check if the same resources are available for the entire walltime
            bool resources_available = true;
            
            // First, ensure all time slots exist
            if(time_index + job->walltime > available_res.size()){
                ensure_time_slot_exists(time_index + job->walltime);
            }
            
            // Then check if all required resources are available at each time slot

            // for (size_t t = time_index + 1; t < time_index + job->walltime; ++t) {
            //     // Check if all required resources are available at this time slot
            //     for (uint32_t res : job_resources) {
            //         if (available_res[t].find(res) == available_res[t].end()) {
            //             resources_available = false;
            //             log_message("    Resource %u not available at time slot %.2f\n", 
            //                    res, static_cast<double>(t));
            //             break;
            //         }
            //     }
                
            //     if (!resources_available) {
            //         break;
            //     }
            // }
            
            if (resources_available) {
                // Erase resources from all time slots that the job will occupy
                for (size_t t = time_index; t < time_index + job->walltime; ++t) {
                    // Erase the resources from this time slot
                    for (uint32_t res : job_resources) {
                        available_res[t].erase(res);
                    }
                }
                
                running_jobs[job->job_id] = job;
                job_allocations[job->job_id] = job_resources;
                
                // Build a comma-separated list of allocated resource IDs.
                std::string resources_str;
                for (auto it = job_resources.begin(); it != job_resources.end(); ++it) {
                    if (it != job_resources.begin())
                        resources_str += ",";
                    resources_str += std::to_string(*it);
                }
                execute_job(job, job_resources);
                
            }
        } else {
            // The front job does not fit: attempt to backfill one job from the rest of the queue.
            
            bool backfilled = false;
            
            for (auto it = std::next(jobs->begin()); it != jobs->end(); ++it) {
                SchedJob* backfill_job = *it;
                
                if (available_res[time_index].size() >= backfill_job->nb_hosts) {
                    
                    std::set<uint32_t> assigned_resources = available_res[time_index];
                    
                    // Sort the assigned resources in ascending order (BASIC)
                    std::vector<uint32_t> sorted_resources(assigned_resources.begin(), assigned_resources.end());
                    std::sort(sorted_resources.begin(), sorted_resources.end());
                    assigned_resources = std::set<uint32_t>(sorted_resources.begin(), sorted_resources.end());
                    
                    uint32_t time = 0;
                    
                    // First, ensure all time slots exist
                    if(time_index + backfill_job->walltime > available_res.size()){
                        ensure_time_slot_exists(time_index + backfill_job->walltime);
                    }
                    
                    
                    //now add an iterator from time_index to the end of the available_res vector
                    for (auto it = time_index; it < time_index + backfill_job->walltime; ++it) {
                        
                        // here we need to take the intersection of the assigned_resources and the available_res[it]
                        std::set<uint32_t> intersection;
                        std::set_intersection(assigned_resources.begin(), assigned_resources.end(), 
                                             available_res[it].begin(), available_res[it].end(), 
                                             std::inserter(intersection, intersection.begin()));
                        
                        assigned_resources = intersection;
                        // update the time variable with the duration of the timeslice j
                        time = time + (it - time_index); //?????
                        // if we arrived at the end of the needed walltime, we can break the loop
                        if (time >= backfill_job->walltime) {
                            backfilled = true;
                            break;
                        }
                    }
                    
                    if(assigned_resources.size() >= backfill_job->nb_hosts && backfilled) {

                        // Find contiguous resources
                        std::vector<uint32_t> best_effort_contiguous_resources;
                        for (auto it = assigned_resources.begin(); it != assigned_resources.end(); ++it) {
                            if (best_effort_contiguous_resources.empty() || *it == best_effort_contiguous_resources.back() + 1) {
                                best_effort_contiguous_resources.push_back(*it);
                            } else {
                                best_effort_contiguous_resources.clear();
                            }
                            if (best_effort_contiguous_resources.size() == backfill_job->nb_hosts) {
                                break;
                            }
                        }
                        
                        // Check if we found enough contiguous resources
                        if (best_effort_contiguous_resources.size() < backfill_job->nb_hosts) {
                            // If we don't have enough contiguous resources, just take the first nb_hosts resources
                            best_effort_contiguous_resources.clear();

                            auto it = assigned_resources.begin();
                            for (uint8_t i = 0; i < backfill_job->nb_hosts && it != assigned_resources.end(); ++i, ++it) {
                                best_effort_contiguous_resources.push_back(*it);
                            }
                            
                            non_contiguous_backfill_count++;
                        } else {
                            contiguous_backfill_count++;
                        }
                        
                        // Use the non-contiguous resources instead
                        std::set<uint32_t> trimmed_resources(best_effort_contiguous_resources.begin(), best_effort_contiguous_resources.end());
                        
                        job_allocations[backfill_job->job_id] = trimmed_resources;
                        running_jobs[backfill_job->job_id] = backfill_job;
                        backfill_success_count++;
                        
                        // Build resource string and execute job
                        std::string resources_str;
                        for (auto res_iter = trimmed_resources.begin(); res_iter != trimmed_resources.end(); ++res_iter) {
                            if (res_iter != trimmed_resources.begin())
                                resources_str += ",";
                            resources_str += std::to_string(*res_iter);
                        }
                        
                        // Only execute if we have a valid resource string
                        if (!resources_str.empty()) {
                            mb->add_execute_job(backfill_job->job_id, resources_str);
                        } else {
                            continue;
                        }
                        
                        // Erase resources from all time slots that the job will occupy
                        for (size_t t = time_index; t < time_index + backfill_job->walltime; ++t) {
                            // Erase the resources from this time slot
                            for (uint32_t res : trimmed_resources) {
                                available_res[t].erase(res);
                            }
                        }
                        
                        // Remove the backfilled job from the pending queue.
                        jobs->erase(it);
                        backfilled = true;
                        
                        break; // Schedule at most one backfilled job in this decision cycle.
                    }
                }
            }
            
            // If no pending job (other than the front) can be scheduled, then break out.
            if (!backfilled) {
                break;
            } else {
                // (Optionally, you could continue the loop to check for more opportunities.)
                break;
            }
        }
    }
    
    log_message("Backfilling statistics: %u total successes (%u contiguous, %u non-contiguous)\n", 
           backfill_success_count, contiguous_backfill_count, non_contiguous_backfill_count);
    
    mb->finish_message(parsed->now());
    serialize_message(*mb, !format_binary, const_cast<const uint8_t **>(decisions), decisions_size);
    return 0;
}

// -------------------------