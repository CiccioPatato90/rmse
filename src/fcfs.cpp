#include <cstdint>
#include <list>

#include <batprotocol.hpp>
#include <intervalset.hpp>

#include <unordered_map>

#include "batsim_edc.h"

using namespace batprotocol;

struct SchedJob {
  std::string job_id;
  uint8_t nb_hosts;
  IntervalSet assigned_resources;
};

MessageBuilder * mb = nullptr;
bool format_binary = true; // whether flatbuffers binary or json format should be used
std::list<SchedJob*> * jobs = nullptr;
uint32_t platform_nb_hosts = 0;
//IntervalSet to model available resources
IntervalSet available_resources;
//Jobs in execution
std::list<SchedJob*> * running_jobs = nullptr;
//HashMap to model running_jobs on resources
//std::unordered_map<std::string, int> running_jobs;


// this function is called by batsim to initialize your decision code
uint8_t batsim_edc_init(const uint8_t * data, uint32_t size, uint32_t flags) {
  format_binary = ((flags & BATSIM_EDC_FORMAT_BINARY) != 0);
  if ((flags & (BATSIM_EDC_FORMAT_BINARY | BATSIM_EDC_FORMAT_JSON)) != flags) {
    printf("Unknown flags used, cannot initialize myself.\n");
    return 1;
  }

  mb = new MessageBuilder(!format_binary);
  jobs = new std::list<SchedJob*>();
  running_jobs = new std::list<SchedJob*>();

  // ignore initialization data
  (void) data;
  (void) size;

  return 0;
}

// this function is called by batsim to deinitialize your decision code
uint8_t batsim_edc_deinit() {
  delete mb;
  mb = nullptr;

  if (jobs != nullptr) {
    for (auto * job : *jobs) {
        delete job;
    }
    delete jobs;
    jobs = nullptr;
  }

    if (running_jobs != nullptr) {
    for (auto * job : *running_jobs) {
        delete job;
    }
    delete running_jobs;
    running_jobs = nullptr;
  }

  return 0;
}

// this function is called by batsim when it thinks that you may take decisions
uint8_t batsim_edc_take_decisions(
  const uint8_t * what_happened,
  uint32_t what_happened_size,
  uint8_t ** decisions,
  uint32_t * decisions_size)
{
  (void) what_happened_size;

  // deserialize the message received
  auto * parsed = deserialize_message(*mb, !format_binary, what_happened);

  // clear data structures to take the next decisions.
  // decisions will now use the current time, as received from batsim
  mb->clear(parsed->now());

  // traverse all events that have just been received
  auto nb_events = parsed->events()->size();
  for (unsigned int i = 0; i < nb_events; ++i) {
    auto event = (*parsed->events())[i];
    printf("fcfs received event type='%s'\n", batprotocol::fb::EnumNamesEvent()[event->event_type()]);
    switch (event->event_type()) {
      // protocol handshake
      case fb::Event_BatsimHelloEvent: {
        mb->add_edc_hello("fcfs", "0.1.0");
      } break;
      // batsim tells you that the simulation starts, providing you various initialization information
      case fb::Event_SimulationBeginsEvent: {

        auto simu_begins = event->event_as_SimulationBeginsEvent();
        platform_nb_hosts = simu_begins->computation_host_number();
        // Init available resources
        available_resources = IntervalSet(IntervalSet::ClosedInterval(0, platform_nb_hosts-1));
      } break;
      // a job has just been submitted
      case fb::Event_JobSubmittedEvent: {
        // this doesn't really change, as when the job arrives we only want to push it on the queue
        // therefore maintaining order of arrival
        auto parsed_job = event->event_as_JobSubmittedEvent();
        auto job = new SchedJob();
        job->job_id = parsed_job->job_id()->str();

        job->nb_hosts = parsed_job->job()->resource_request();
        // if you want to initialize the IntervalSet, do it here
        if (job->nb_hosts > platform_nb_hosts) {
          mb->add_reject_job(job->job_id);
          delete job;
        }
        else {
          jobs->push_back(job);
        }
      } break;
      // a job has just completed
      case fb::Event_JobCompletedEvent: {
        auto parsed_event = event->event_as_JobCompletedEvent();  
        
        //retrieves info about job
        auto job_it = running_jobs->begin();
        SchedJob* finished_job = nullptr;
        while(job_it != running_jobs->end()){
          if((*job_it)->job_id == (std::string)parsed_event->job_id()->str()){
            finished_job = *job_it;
            running_jobs->erase(job_it);
            break;
          }
          ++job_it;
        }
        
        auto allocated = finished_job->assigned_resources;
        available_resources += allocated;
      } break;
      default: break;
    }
  }

  // First Come First Served will run jobs in queue order if the host is ready and the queue is not empty
  while (!jobs->empty()) {
    SchedJob* new_job = jobs->front();
    //  THIS SHOULD NOT BE EXECUTED SINCE WE DON'T WANT TO REMOVE THE JOB IF WE ARE NOT SURE
  
    if((new_job->nb_hosts) <= available_resources.size()){
      //  it fits since we are retrieving the needed resources
      //  it's enough to check >= 0, since we ahve already taken the first nb_hosts elements
      IntervalSet assigned_hosts = available_resources.left(new_job->nb_hosts);
      
      //  Update SchedJob with assigned resources
      new_job->assigned_resources = assigned_hosts;
      //  EXECUTE JOBS
      mb->add_execute_job(new_job->job_id, assigned_hosts.to_string_hyphen());
      //  update running jobs map
      //  running_jobs[new_job->job_id] = assigned_hosts.to_string_hyphen();
      running_jobs->push_back(new_job);
      available_resources -= assigned_hosts;
      jobs->pop_front();
    }else{
      //it doesn't fit, don't do anythign
      break;
    }
  }

  // serialize decisions that have been taken into the output parameters of the function (decisions, decisions_size)
  mb->finish_message(parsed->now());
  serialize_message(*mb, !format_binary, const_cast<const uint8_t **>(decisions), decisions_size);
  return 0;
}
