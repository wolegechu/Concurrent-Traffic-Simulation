#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <random>
#include <future>

#include "TrafficLight.h"



template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    std::unique_lock<std::mutex> lock(_mutex);
    _condition.wait(lock, [this]{ return  !_queue.empty();});

    T msg = std::move(_queue.front());
    _queue.pop_front();
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    std::lock_guard<std::mutex> lock(_mutex);
    _queue.emplace_back(msg);
    _condition.notify_one();
}

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto msg = _queue.receive();
        if(msg == green){
            return;
        }
    }
    
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(4000, 6000);
    int cycleDuration = dist(gen);

    auto lastSwitchedTime = std::chrono::system_clock::now();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        auto tmpSeconds = std::chrono::duration_cast<std::chrono::milliseconds>
                                (std::chrono::system_clock::now() - lastSwitchedTime);
        int durationSinceSwitched = tmpSeconds.count();
        
        if(durationSinceSwitched >= cycleDuration){
            _currentPhase = _currentPhase == red ? green : red;

            auto sentFuture = std::async(std::launch::async, 
                                    &MessageQueue<TrafficLightPhase>::send,
                                    &_queue,
                                    std::move(_currentPhase));
            sentFuture.wait();

            lastSwitchedTime = std::chrono::system_clock::now();
            cycleDuration = dist(gen);
        }
        
    }
    
}
