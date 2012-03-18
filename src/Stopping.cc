/** \class Stopping
 *
 *  \author A. Spataru - andrei.cristian.spataru@cern.ch
 */

#include "EventFilter/ResourceBroker/interface/RBStateMachine.h"
//#include "EventFilter/ResourceBroker/interface/IPCMethod.h"
#include "EventFilter/ResourceBroker/interface/SharedResources.h"

#include <iostream>
#include <vector>

using std::cout;
using std::endl;
using std::vector;
using namespace evf::rb_statemachine;

// entry action, state notification, state action
//______________________________________________________________________________
void Stopping::do_entryActionWork() {
}

void Stopping::do_stateNotify() {
	SharedResourcesPtr_t res = outermost_context().getSharedResources();
	LOG4CPLUS_INFO(res->log_, "--> ResourceBroker: NEW STATE: " << stateName());
	outermost_context().setExternallyVisibleState(stateName());
	outermost_context().setInternalStateName(stateName());
	// notify RCMS of the new state
	outermost_context().rcmsStateChangeNotify();
}

void Stopping::do_stateAction() const {
	SharedResourcesPtr_t res = outermost_context().getSharedResources();
	IPCMethod* resourceStructure = res->resourceStructure_;

	try {
		LOG4CPLUS_INFO(res->log_, "Start stopping :) ...");
		resourceStructure->shutDownClients();
		timeval now;
		timeval then;
		gettimeofday(&then, 0);
		while (!resourceStructure->isReadyToShutDown()) {
			::usleep(res->resourceStructureTimeout_.value_ * 10);
			gettimeofday(&now, 0);
			if ((unsigned int) (now.tv_sec - then.tv_sec)
					> res->resourceStructureTimeout_.value_ / 10000) {
				cout << "times: " << now.tv_sec << " " << then.tv_sec << " "
						<< res->resourceStructureTimeout_.value_ / 10000
						<< endl;
				LOG4CPLUS_WARN(res->log_,
						"Some Process did not detach - going to Emergency stop!");
				emergencyStop();
				break;
			}
		}

		if (resourceStructure->isReadyToShutDown()) {
			LOG4CPLUS_INFO(res->log_, "Finished stopping!");
			EventPtr stopDone(new StopDone());
			res->commands_.enqEvent(stopDone);
		}
	} catch (xcept::Exception &e) {
		res->reasonForFailed_ = e.what();
		moveToFailedState(e);
	}
}

/*
 * I2O capability
 */
bool Stopping::discardDataEvent(MemRef_t* bufRef) const {
	SharedResourcesPtr_t res = outermost_context().getSharedResources();
	return res->resourceStructure_->discardDataEvent(bufRef);
}
bool Stopping::discardDqmEvent(MemRef_t* bufRef) const {
	SharedResourcesPtr_t res = outermost_context().getSharedResources();
	return res->resourceStructure_->discardDqmEvent(bufRef);
}


// construction / destruction
//______________________________________________________________________________
Stopping::Stopping(my_context c) :
	my_base(c) {
	safeEntryAction();
}

Stopping::~Stopping() {
	safeExitAction();
}

void Stopping::emergencyStop() const {
	SharedResourcesPtr_t res = outermost_context().getSharedResources();
	IPCMethod* resourceStructure = res->resourceStructure_;

	LOG4CPLUS_WARN(res->log_, "in Emergency stop - handle non-clean stops");
	vector<pid_t> client_prc_ids = resourceStructure->clientPrcIds();
	for (UInt_t i = 0; i < client_prc_ids.size(); i++) {
		pid_t pid = client_prc_ids[i];
		cout << "B: killing process " << i << " pid= " << pid << endl;
		if (pid != 0) {
			//assume processes are dead by now
			if (!resourceStructure->handleCrashedEP(res->runNumber_, pid))
				res->nbTimeoutsWithoutEvent_++;
			else
				res->nbTimeoutsWithEvent_++;
		}
	}
	resourceStructure->lastResort();
	::sleep(1);
	if (!resourceStructure->isReadyToShutDown()) {
		res->reasonForFailed_
				= "EmergencyStop: failed to shut down ResourceTable";
		XCEPT_RAISE(evf::Exception, res->reasonForFailed_);
	}
	res->printWorkLoopStatus();
	res->lock();

	delete res->resourceStructure_;
	res->resourceStructure_ = 0;

	cout << "cycle through resourcetable config " << endl;
	res->configureResources(outermost_context().getApp());
	res->unlock();
	if (res->shmInconsistent_)
		XCEPT_RAISE(evf::Exception, "Inconsistent shm state");
	cout << "done with emergency stop" << endl;
}

// exit action, state name, move to failed state
//______________________________________________________________________________
void Stopping::do_exitActionWork() {
}

string Stopping::do_stateName() const {
	return std::string("Stopping");
}

void Stopping::do_moveToFailedState(xcept::Exception& exception) const {
	SharedResourcesPtr_t res = outermost_context().getSharedResources();
	LOG4CPLUS_ERROR(res->log_,
			"Moving to FAILED state! Reason: " << exception.what());
	EventPtr fail(new Fail());
	res->commands_.enqEvent(fail);
}
