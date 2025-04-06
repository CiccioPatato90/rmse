# Batsim Implementation Plan

## General Structure of a Batsim Program

A Batsim program follows an event-driven architecture with these main components:

### 1. Data Structures
```cpp
// Core job structure
struct SchedJob {
    std::string job_id;
    uint8_t nb_hosts;
    double walltime;      // Expected runtime
    double submit_time;   // When job was submitted
    double start_time;    // Predicted/actual start time
    std::set<uint32_t> allocated_resources;  // Resources assigned to job
};

// Global state
static MessageBuilder *mb = nullptr;
static bool format_binary = true;
static std::list<SchedJob*> *jobs = nullptr;  // Pending jobs queue
static std::unordered_map<std::string, SchedJob*> running_jobs;
static std::set<uint32_t> available_res;  // Available resources

// Additional global state
static double current_time = 0.0;
static std::unordered_map<std::string, double> job_reservations;  // Maps job_id to reserved start time
```

### 2. Core Functions
```cpp
// Initialization
extern "C" uint8_t batsim_edc_init(const uint8_t *data, uint32_t size, uint32_t flags) {
    // Initialize data structures
    // Set up message builder
    // Configure format (binary/JSON)
}

// Cleanup
extern "C" uint8_t batsim_edc_deinit() {
    // Clean up allocated memory
    // Reset state
}

// Main decision function
extern "C" uint8_t batsim_edc_take_decisions(
    const uint8_t *what_happened,
    uint32_t what_happened_size,
    uint8_t **decisions,
    uint32_t *decisions_size) {
    
    // 1. Process events
    // 2. Update system state
    // 3. Make scheduling decisions
    // 4. Return decisions to Batsim
}
```

### 3. Event Processing
```cpp
switch (event->event_type()) {
    case fb::Event_BatsimHelloEvent:
        // Initial handshake
        break;
    case fb::Event_SimulationBeginsEvent:
        // Platform initialization
        break;
    case fb::Event_JobSubmittedEvent: {
        auto parsed_job = event->event_as_JobSubmittedEvent();
        auto job = new SchedJob();
        job->job_id = parsed_job->job_id()->str();
        job->nb_hosts = parsed_job->job()->resource_request();
        job->walltime = parsed_job->job()->walltime();  // Get walltime from job
        job->submit_time = current_time;
        
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
        
        if (running_jobs.count(completed_job_id)) {
            SchedJob* completed_job = running_jobs[completed_job_id];
            // Release resources
            for (uint32_t res : completed_job->allocated_resources) {
                available_res.insert(res);
            }
            running_jobs.erase(completed_job_id);
            delete completed_job;
        }
    } break;
}
```

## Implementation Plan for Thesis Algorithms

EASY BACKFILL PSEUDO CODE (Working version and BASELINE):
input:
- list of queued jobs with no des and time requirements
- list of running jobs with no de usage and exp ected termination times
- numb er of free no des
algorithm EASY back ll:
1. nd the shadow time and extra no des
(a) sort the list of running jobs according to their exp ected termination time
(b) lo op over the list and collect no des until the numb er of available no des
is sucient for the rst job in the queue
(c) the time at which this happ ens is the shadow time
(d) if at this time more no des are available than needed by the rst queued
job, the ones left over are the extra no des
2. nd a back ll job
(a) lo op on the list of queued jobs in order of arrival
(b) for each one, check whether either of the following conditions hold:
i. it requires no more than the currently free no des, and will terminate
by the shadow time, or
ii. it requires no more than the minimum of the currently free no des
and the extra no des
(c) the rst such job can be used for back filling


BASIC (CONSERVATIVE) BACKFILLING PSEUDO CODE (TO BE IMPLEMENTED):
input:
- list of queued jobs with no des and time requirements
- list of running jobs with no de usage and exp ected termination times
- numb er of free nodes
algorithm conservative back ll from scratch:
1. generate pro cessor usage pro le of running jobs
(a) sort the list of running jobs according to their exp ected termination time
(b) lo op over the list dividing the future into p erio ds according to job ter-
minations, and list the numb er of pro cessors used in each p erio d; this is
the usage pro le
2. try to back ll with queued jobs
(a) lo op on the list of queued jobs in order of arrival
(b) for each one, scan the pro le and nd the rst p oint where enough
pro cessors are available to run this job. this is called the anchor p oint
i. starting from this p oint, continue scanning the pro le to ascertain
that the pro cessors remain available until the job's exp ected termi-
nation
ii. if so, up date the pro le to re ect the allo cation of pro cessors to this
job
iii. if not, continue the scan to nd the next p ossible anchor p oint, and
rep eat the check
(c) the rst job found that can start immediately is used for back lling 

