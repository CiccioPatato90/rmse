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
    
    // Open log file
    log_file.open("conservative_backfill_log.txt", std::ios::out | std::ios::trunc);
    if (!log_file.is_open()) {
        printf("Warning: Could not open log file for writing\n");
    } else {
        log_file << "Conservative Backfilling Scheduler Log\n";
        log_file << "===================================\n\n";
    }
    
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
    
    // Close log file
    if (log_file.is_open()) {
        log_file.close();
    }
    
    return 0;
}

// Helper function to log to both console and file
void log_message(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Print to console
    printf("%s", buffer);
    
    // Write to log file if open
    if (log_file.is_open()) {
        log_file << buffer;
        log_file.flush();  // Ensure it's written immediately
    }
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

// Helper function to print the reservation table
void print_reservation_table(double current_time) {
    log_message("\nRESERVATION TABLE (time: %.2f):\n", current_time);
    
    // Find the maximum time slot we need to display
    size_t max_time_slot = available_res.size();
    if (max_time_slot == 0) {
        log_message("  Empty reservation table\n");
        return;
    }
    
    // For each time slot, print available and allocated resources
    for (size_t t = 0; t < max_time_slot; ++t) {
        // Get all resources that are allocated at this time
        std::set<uint32_t> allocated_res;
        for (uint32_t i = 0; i < platform_nb_hosts; ++i) {
            if (available_res[t].find(i) == available_res[t].end()) {
                allocated_res.insert(i);
            }
        }
        
        log_message("  Time slot %.2f: %zu/%u resources available (", 
               static_cast<double>(t), available_res[t].size(), platform_nb_hosts);
        
        // Print available resources
        for (auto res : available_res[t]) {
            log_message("%u ", res);
        }
        log_message(")\n");
        
        // Print allocated resources if any
        if (!allocated_res.empty()) {
            log_message("    Allocated resources: ");
            for (auto res : allocated_res) {
                log_message("%u ", res);
            }
            log_message("\n");
        }
    }
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
        log_message("conservative_backfilling received event type='%s'\n", fb::EnumNamesEvent()[event->event_type()]);
        
        switch (event->event_type()) {
            case fb::Event_BatsimHelloEvent: {
                mb->add_edc_hello("conservative_backfilling", "1.0.0");
            } break;
            
            case fb::Event_SimulationBeginsEvent: {
                auto simu_begins = event->event_as_SimulationBeginsEvent();
                platform_nb_hosts = simu_begins->computation_host_number();
                log_message("Simulation started with %u hosts\n", platform_nb_hosts);
                
                // Initialize available resources for time 0 (hosts are numbered from 0 to platform_nb_hosts-1)
                ensure_time_slot_exists(0);
                
                print_reservation_table(current_time);
            } break;
            
            case fb::Event_JobSubmittedEvent: {
                auto parsed_job = event->event_as_JobSubmittedEvent();
                auto job = new SchedJob();
                job->job_id = parsed_job->job_id()->str();
                job->nb_hosts = parsed_job->job()->resource_request();
                job->walltime = parsed_job->job()->walltime();  // Initialize walltime from the job
                
                // Reject jobs that request more hosts than available on the platform
                if (job->nb_hosts > platform_nb_hosts) {
                    log_message("Job %s requests more hosts (%u) than available on the platform (%u). Rejecting.\n", 
                           job->job_id.c_str(), job->nb_hosts, platform_nb_hosts);
                    mb->add_reject_job(job->job_id);
                    delete job;
                } else {
                    jobs->push_back(job);
                    log_message("Job %s added to queue (queue size: %zu)\n", job->job_id.c_str(), jobs->size());
                }
            } break;
            
            case fb::Event_JobCompletedEvent: {
                auto parsed_job = event->event_as_JobCompletedEvent();
                std::string completed_job_id = parsed_job->job_id()->str();
                log_message("Job %s completed, freeing %zu resources\n", 
                       completed_job_id.c_str(), job_allocations[completed_job_id].size());
                
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
                    
                    log_message("Resources freed for job %s. Updated reservation table:\n", completed_job_id.c_str());
                    print_reservation_table(current_time);
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
    
    log_message("\n===== SCHEDULING CYCLE START (time: %.2f) =====\n", current_time);
    
    // Print queue state
    log_message("Queue state (%zu jobs):\n", jobs->size());
    int queue_position = 0;
    for (auto job : *jobs) {
        log_message("  [%d] Job %s: %u hosts, walltime %.2f\n", 
               queue_position++, job->job_id.c_str(), job->nb_hosts, static_cast<double>(job->walltime));
    }
    
    // Print running jobs
    log_message("Running jobs (%zu):\n", running_jobs.size());
    for (auto& pair : running_jobs) {
        log_message("  Job %s: %u hosts on resources (", pair.first.c_str(), pair.second->nb_hosts);
        for (auto res : job_allocations[pair.first]) {
            log_message("%u ", res);
        }
        log_message(")\n");
    }
    
    // Print reservation table
    print_reservation_table(current_time);
    
    while (!jobs->empty()) {
        // Always try to schedule the job at the front of the queue first.
        SchedJob* job = jobs->front();
        log_message("\nTrying to schedule first job in queue: %s (needs %u hosts, walltime %.2f)\n", 
               job->job_id.c_str(), job->nb_hosts, static_cast<double>(job->walltime));
        
        if (available_res[time_index].size() >= job->nb_hosts) {
            // The front job fits: allocate the first nb_hosts resources available.
            log_message("  SUCCESS: Job %s can be scheduled immediately with %u hosts\n", 
                   job->job_id.c_str(), job->nb_hosts);
            
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
                log_message("  Creating new entry in reservation with parameter %u hosts\n", 
                    time_index + job->walltime);
                ensure_time_slot_exists(time_index + job->walltime);
                
                print_reservation_table(current_time);
            }
            
            // Then check if all required resources are available at each time slot

            for (size_t t = time_index + 1; t < time_index + job->walltime; ++t) {
                // Check if all required resources are available at this time slot
                for (uint32_t res : job_resources) {
                    if (available_res[t].find(res) == available_res[t].end()) {
                        resources_available = false;
                        log_message("    Resource %u not available at time slot %.2f\n", 
                               res, static_cast<double>(t));
                        break;
                    }
                }
                
                if (!resources_available) {
                    break;
                }
            }
            
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
                mb->add_execute_job(job->job_id, resources_str);
                
                jobs->pop_front();
                
                log_message("  Job %s scheduled on resources: %s\n", job->job_id.c_str(), resources_str.c_str());
                log_message("  Remaining available resources: %zu\n", available_res[time_index].size());
                
                // Print updated reservation table
                print_reservation_table(current_time);
            } else {
                log_message("  FAILED: Job %s cannot be scheduled (resources not available for entire walltime)\n", 
                       job->job_id.c_str());
            }
        } else {
            // The front job does not fit: attempt to backfill one job from the rest of the queue.
            log_message("  FAILED: Job %s cannot be scheduled immediately (needs %u hosts, only %zu available)\n", 
                   job->job_id.c_str(), job->nb_hosts, available_res[time_index].size());
            log_message("  Attempting to backfill a job from the rest of the queue...\n");
            
            bool backfilled = false;
            
            for (auto it = std::next(jobs->begin()); it != jobs->end(); ++it) {
                SchedJob* backfill_job = *it;
                log_message("  Checking if job %s (needs %u hosts, walltime %.2f) can be backfilled\n", 
                       backfill_job->job_id.c_str(), backfill_job->nb_hosts, static_cast<double>(backfill_job->walltime));
                
                if (available_res[time_index].size() >= backfill_job->nb_hosts) {
                    log_message("    SUCCESS: Job %s can be backfilled with %u hosts\n", 
                           backfill_job->job_id.c_str(), backfill_job->nb_hosts);
                    
                    std::set<uint32_t> assigned_resources = available_res[time_index];
                    
                    // Sort the assigned resources in ascending order (BASIC)
                    std::vector<uint32_t> sorted_resources(assigned_resources.begin(), assigned_resources.end());
                    std::sort(sorted_resources.begin(), sorted_resources.end());
                    assigned_resources = std::set<uint32_t>(sorted_resources.begin(), sorted_resources.end());
                    
                    uint32_t time = 0;
                    
                    // First, ensure all time slots exist
                    if(time_index + backfill_job->walltime > available_res.size()){
                        log_message("  Creating new entry in reservation with parameter %u hosts\n", 
                            time_index + backfill_job->walltime);
                        ensure_time_slot_exists(time_index + backfill_job->walltime);
                        print_reservation_table(current_time);
                    }
                    
                    // Log initial assigned resources
                    log_message("    Initial assigned resources (%zu): ", assigned_resources.size());
                    for (auto res : assigned_resources) {
                        log_message("%u ", res);
                    }
                    log_message("\n");
                    
                    //now add an iterator from time_index to the end of the available_res vector
                    for (auto it = time_index; it < time_index + backfill_job->walltime; ++it) {
                        // Log available resources at this time slot
                        log_message("    Time slot %.2f - Available resources (%zu): ", 
                               static_cast<double>(it), available_res[it].size());
                        for (auto res : available_res[it]) {
                            log_message("%u ", res);
                        }
                        log_message("\n");
                        
                        // here we need to take the intersection of the assigned_resources and the available_res[it]
                        std::set<uint32_t> intersection;
                        std::set_intersection(assigned_resources.begin(), assigned_resources.end(), 
                                             available_res[it].begin(), available_res[it].end(), 
                                             std::inserter(intersection, intersection.begin()));
                        
                        // Log intersection results
                        log_message("    After intersection - Resources (%zu): ", intersection.size());
                        for (auto res : intersection) {
                            log_message("%u ", res);
                        }
                        log_message("\n");
                        
                        assigned_resources = intersection;
                        // update the time variable with the duration of the timeslice j
                        time = time + (it - time_index); //?????
                        // if we arrived at the end of the needed walltime, we can break the loop
                        if (time >= backfill_job->walltime) {
                            backfilled = true;
                            log_message("    Reached required walltime (%.2f), breaking loop\n", 
                                   static_cast<double>(backfill_job->walltime));
                            break;
                        }
                    }
                    
                    if(assigned_resources.size() >= backfill_job->nb_hosts && backfilled) {
                        log_message("    SUCCESS: Job %s can be backfilled with %u hosts\n", 
                               backfill_job->job_id.c_str(), backfill_job->nb_hosts);

                        // Get only the first backfill_job->nb_hosts resources
                        std::set<uint32_t> trimmed_resources(assigned_resources.begin(), std::next(assigned_resources.begin(), std::min(static_cast<size_t>(backfill_job->nb_hosts), assigned_resources.size())));
                        job_allocations[backfill_job->job_id] = trimmed_resources;
                        running_jobs[backfill_job->job_id] = backfill_job;

                        // Erase resources from all time slots that the job will occupy
                        for (size_t t = time_index; t < time_index + backfill_job->walltime; ++t) {
                            // Erase the resources from this time slot
                            for (uint32_t res : trimmed_resources) {
                                available_res[t].erase(res);
                            }
                        }
                        
                        
                        std::string resources_str;
                        for (auto res_iter = trimmed_resources.begin(); res_iter != trimmed_resources.end(); ++res_iter) {
                            if (res_iter != trimmed_resources.begin())
                                resources_str += ",";
                            resources_str += std::to_string(*res_iter);
                        }
                        mb->add_execute_job(backfill_job->job_id, resources_str);
                        
                        // Remove the backfilled job from the pending queue.
                        jobs->erase(it);
                        backfilled = true;
                        
                        print_reservation_table(current_time);
                        break; // Schedule at most one backfilled job in this decision cycle.
                    } else {
                        log_message("    FAILED: Job %s cannot be backfilled (", backfill_job->job_id.c_str());
                        if (assigned_resources.size() < backfill_job->nb_hosts) {
                            log_message("insufficient resources after intersection: %zu < %u", 
                                   assigned_resources.size(), backfill_job->nb_hosts);
                        } else {
                            log_message("did not reach required walltime: %u < %.2f", 
                                   time, static_cast<double>(backfill_job->walltime));
                        }
                        log_message(")\n");
                    }
                } else {
                    log_message("    FAILED: Job %s cannot be backfilled (needs %u hosts, only %zu available)\n", 
                           backfill_job->job_id.c_str(), backfill_job->nb_hosts, available_res[time_index].size());
                }
            }
            
            // If no pending job (other than the front) can be scheduled, then break out.
            if (!backfilled) {
                log_message("  No jobs could be backfilled, ending scheduling cycle\n");
                break;
            } else {
                log_message("  A job was backfilled, ending scheduling cycle\n");
                // (Optionally, you could continue the loop to check for more opportunities.)
                break;
            }
        }
    }
    
    log_message("===== SCHEDULING CYCLE END =====\n\n");
    
    mb->finish_message(parsed->now());
    serialize_message(*mb, !format_binary, const_cast<const uint8_t **>(decisions), decisions_size);
    return 0;
}

// -------------------------