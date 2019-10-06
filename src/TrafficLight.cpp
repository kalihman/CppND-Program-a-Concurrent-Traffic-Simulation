#include <iostream>
#include <random>
#include "TrafficLight.h"
#include <queue>
#include <future>

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    std::unique_lock<std::mutex> uniqueLock(_mutex);
    _condition.wait(uniqueLock, [this] { return !_queue.empty(); });

    T message = std::move(_queue.back());
    _queue.pop_back();
    return message;
}

template <typename T>
void MessageQueue<T>::send(T &&message)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    std::lock_guard<std::mutex> lockGuard(_mutex);
    _queue.push_back(std::move(message));
    _condition.notify_one();
}

/* Implementation of class "TrafficLight" */
TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _messageQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen() const
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(true)
    {
        /* Wait until traffic light is green */
        auto currentPhase = _messageQueue->receive();
        if(currentPhase == TrafficLightPhase::green) return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase() const
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    /* Generate random number between 4 to 6 */
    std::random_device randomDevice;
    std::mt19937_64 engine(randomDevice());
    std::uniform_real_distribution<> distribution(4.0, 6.0);

    /* Init cycle duration */
    int cycleDuration = distribution(engine);

    /* Start timer */
    auto lastUpdate = std::chrono::system_clock::now();
    while(true) 
    {
        /* Time diff to timer */
        long sinceLastUpdate = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - lastUpdate
        ).count();

        /* Toggle traffic light */
        if (sinceLastUpdate >= cycleDuration) 
        {
            /* Toggle phase */
            if(_currentPhase == red) _currentPhase = TrafficLightPhase::green;
            else _currentPhase = TrafficLightPhase::red;

            auto message = _currentPhase;
            auto isSent = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _messageQueue, std::move(message));
            isSent.wait();

            /* Reset timer */
            lastUpdate = std::chrono::system_clock::now();

            /* Change cycle duration */
            cycleDuration = distribution(engine);
        }


    }
}