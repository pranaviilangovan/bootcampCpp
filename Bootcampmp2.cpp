#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <stdexcept>

using namespace std;

// Forward declarations
class ServiceCenter;

// Observer Pattern - Interface for notifications
class ServiceObserver {
public:
    virtual void update(const string& message) = 0;
    virtual ~ServiceObserver() = default;
};

// Client class that implements the observer
class Client : public ServiceObserver {
private:
    string name;
    string contact;

public:
    Client(const string& name, const string& contact) 
        : name(name), contact(contact) {}

    void update(const string& message) override {
        cout << "Notification for " << name << ": " << message << endl;
    }

    string getName() const { return name; }
};

// State Pattern - Service States
class ServiceState {
public:
    virtual string getStatus() = 0;
    virtual void nextState(class ServiceAppointment* appointment) = 0;
    virtual ~ServiceState() = default;
};

class ScheduledState : public ServiceState {
public:
    string getStatus() override { return "Scheduled"; }
    void nextState(ServiceAppointment* appointment) override;
};

class InProgressState : public ServiceState {
public:
    string getStatus() override { return "In Progress"; }
    void nextState(ServiceAppointment* appointment) override;
};

class CompletedState : public ServiceState {
public:
    string getStatus() override { return "Completed"; }
    void nextState(ServiceAppointment* appointment) override;
};

// Base Service class
class Service {
protected:
    string serviceType;
    double baseCost;
    vector<string> partsRequired;

public:
    Service(const string& type, double cost) 
        : serviceType(type), baseCost(cost) {}

    virtual double calculateCost() = 0;
    virtual string getDescription() const = 0;
    virtual ~Service() = default;
};

// Derived Service classes
class OilChange : public Service {
public:
    OilChange() : Service("Oil Change", 50.0) {
        partsRequired = {"Oil Filter", "Engine Oil"};
    }

    double calculateCost() override { return baseCost; }
    string getDescription() const override {
        return "Standard Oil Change Service";
    }
};

class EngineRepair : public Service {
private:
    string repairType;

public:
    EngineRepair(const string& type) 
        : Service("Engine Repair", 200.0), repairType(type) {
        partsRequired = {"Engine Parts", "Lubricants"};
    }

    double calculateCost() override { return baseCost * 1.5; }
    string getDescription() const override {
        return "Engine Repair: " + repairType;
    }
};

// Service Appointment class
class ServiceAppointment {
private:
    shared_ptr<Client> client;
    string vehicleNumber;
    shared_ptr<Service> service;
    string scheduledDate;
    unique_ptr<ServiceState> currentState;
    ServiceCenter* serviceCenter;

public:
    ServiceAppointment(shared_ptr<Client> client, const string& vehicleNum,
                      shared_ptr<Service> service, const string& date,
                      ServiceCenter* center)
        : client(client), vehicleNumber(vehicleNum), service(service),
          scheduledDate(date), serviceCenter(center) {
        currentState = make_unique<ScheduledState>();
    }

    void setState(unique_ptr<ServiceState> newState) {
        currentState = move(newState);
    }

    void progressState() {
        currentState->nextState(this);
    }

    string getStatus() const {
        return currentState->getStatus();
    }

    shared_ptr<Client> getClient() const { return client; }
    string getVehicleNumber() const { return vehicleNumber; }
    shared_ptr<Service> getService() const { return service; }
    string getScheduledDate() const { return scheduledDate; }
};

// Service Center class
class ServiceCenter {
private:
    vector<shared_ptr<ServiceAppointment>> appointments;
    mutex appointmentMutex;
    condition_variable cv;
    bool isOpen;

public:
    ServiceCenter() : isOpen(true) {}

    void addAppointment(shared_ptr<Client> client, const string& vehicleNum,
                       shared_ptr<Service> service, const string& date) {
        lock_guard<mutex> lock(appointmentMutex);
        
        // Check for scheduling conflicts
        for (const auto& apt : appointments) {
            if (apt->getScheduledDate() == date && 
                apt->getVehicleNumber() == vehicleNum) {
                throw runtime_error("Scheduling conflict: Vehicle already has an appointment on this date");
            }
        }

        auto appointment = make_shared<ServiceAppointment>(
            client, vehicleNum, service, date, this);
        appointments.push_back(appointment);
        
        // Notify client
        client->update("Appointment scheduled for " + date);
    }

    void viewAppointments() {
        lock_guard<mutex> lock(appointmentMutex);
        for (const auto& apt : appointments) {
            cout << "\nVehicle: " << apt->getVehicleNumber() 
                 << "\nClient: " << apt->getClient()->getName()
                 << "\nService: " << apt->getService()->getDescription()
                 << "\nDate: " << apt->getScheduledDate()
                 << "\nStatus: " << apt->getStatus() << endl;
        }
    }
};

// State Pattern implementations
void ScheduledState::nextState(ServiceAppointment* appointment) {
    appointment->setState(make_unique<InProgressState>());
}

void InProgressState::nextState(ServiceAppointment* appointment) {
    appointment->setState(make_unique<CompletedState>());
}

void CompletedState::nextState(ServiceAppointment* appointment) {
    // Final state - no transition
}

int main() {
    ServiceCenter serviceCenter;
    int option;

    while (true) {
        cout << "\nVehicle Service Center Management\n"
             << "1. Schedule New Appointment\n"
             << "2. View Appointments\n"
             << "3. Exit\n"
             << "Enter your choice: ";
        
        cin >> option;
        cin.ignore();

        try {
            if (option == 1) {
                string clientName, contact, vehicleNum, date, serviceType;
                
                cout << "Enter client name: ";
                getline(cin, clientName);
                cout << "Enter contact number: ";
                getline(cin, contact);
                cout << "Enter vehicle number: ";
                getline(cin, vehicleNum);
                cout << "Enter appointment date (DD-MM-YYYY): ";
                getline(cin, date);
                cout << "Enter service type (oil/engine): ";
                getline(cin, serviceType);

                auto client = make_shared<Client>(clientName, contact);
                shared_ptr<Service> service;
                
                if (serviceType == "oil") {
                    service = make_shared<OilChange>();
                } else if (serviceType == "engine") {
                    cout << "Enter engine repair type: ";
                    string repairType;
                    getline(cin, repairType);
                    service = make_shared<EngineRepair>(repairType);
                } else {
                    throw invalid_argument("Invalid service type");
                }

                serviceCenter.addAppointment(client, vehicleNum, service, date);
                cout << "Appointment scheduled successfully!\n";
            }
            else if (option == 2) {
                serviceCenter.viewAppointments();
            }
            else if (option == 3) {
                cout << "Exiting system...\n";
                break;
            }
            else {
                cout << "Invalid option!\n";
            }
        }
        catch (const exception& e) {
            cerr << "Error: " << e.what() << endl;
        }
    }

    return 0;
}