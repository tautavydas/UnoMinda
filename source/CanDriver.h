#pragma once

#include <thread>
#include <iostream>
#include <stdint.h>
#include <chrono>
#include <mutex>
#include <condition_variable>

void IRQ_MESSAGE_AVAILABLE_RX() {}
void IRQ_CAN_SEND_MESSAGE_TX() {}

using namespace std;

uint16_t CTRL{};
uint8_t DATA_RX[11]{};
uint8_t DATA_TX[11]{};

bool getFlag2() {return DATA_RX[3] & 0b100;}
uint8_t getCounter() {return DATA_TX[0];}
uint8_t increasedCounter(uint8_t& counter) {return ++counter % 16;}

mutex mtx;
condition_variable cv;

struct CanFrame {
  uint8_t data[8]{};
  uint8_t size{};
  uint16_t id{};

  void write_to_buffer(uint8_t (&buffer)[11]) {
    for (size_t i{0}; i < 8; ++i)
      buffer[i] = data[i];
    buffer[ 8] = size;
    buffer[ 9] = id & 0b0000'0000'1111'1111;
    buffer[10] = id & 0b1111'1111'0000'0000;
  }
};

class CanDriver {
  public:
  enum class Mode : uint8_t {
    Polling,
    ConditionVariable
  } mutable mode;

  void set_RX_IRQ() const {CTRL |= 0b1;}

  void unset_RX_IRQ() const {CTRL &= ~0b1;}

  bool get_RX_IRQ() const {return CTRL & 0b1;}

  void set_TX_IRQ() const {CTRL |= 0b10;}

  void set_TX_AVL() const {CTRL |= 0b100;}

  void set_RX_AVL() const {CTRL |= 0b1000;}

  void unset_RX_AVL() const {CTRL &= ~0b1000;}

  bool get_RX_AVL() const {return CTRL & 0b1000;}

  void start(Mode const val) const {
    mode = val;
    CTRL |= 0b10000;
  }

  void stop() const {CTRL &= ~0b10000;}

  bool isRunning() const {return CTRL & 0b10000;}

  void DataReceive() const {
    switch (mode) {
      case Mode::ConditionVariable:
        Receive_usingConditionVariable();
        break;
      case Mode::Polling:
        Receive_usingPolling();
        break;
    }
  }

  void DataSend() const {
    switch (mode) {
      case Mode::ConditionVariable:
        Send_usingConditionVariable();
        break;
      case Mode::Polling:
        Send_usingPolling();
        break;
    }
  }

  private:
  void Receive_usingConditionVariable() const {
    while (isRunning()) {
      {
        lock_guard lock(mtx);

        uint8_t flag2 = rand() % 2 == 0 ? 0b0000'0100 : 0b0000'0000;
        CanFrame frame{{0, 0, 0, flag2, 0, 0, 0, 0}, 4, 0x300};

        frame.write_to_buffer(DATA_RX);

        set_RX_IRQ();

        cout << "ConditionVariable | Frame Received: ID == " << hex << "0x" << frame.id << ", Flag2 == " << boolalpha << getFlag2() << endl;
      }
      cv.notify_one();
      IRQ_MESSAGE_AVAILABLE_RX();
      this_thread::sleep_for(chrono::milliseconds(10));
    }
  }

  
  void Receive_usingPolling() const {
    while (isRunning()) {
      {
        lock_guard lock(mtx);

        uint8_t flag2 = rand() % 2 == 0 ? 0b0000'0100 : 0b0000'0000;
        CanFrame frame{{0, 0, 0, flag2, 0, 0, 0, 0}, 4, 0x300};

        frame.write_to_buffer(DATA_RX);

        set_RX_AVL();

        cout << "Polling | Frame Received: ID == " << hex << "0x" << frame.id << ", Flag2 == " << boolalpha << getFlag2() << endl;
      }
      this_thread::sleep_for(chrono::milliseconds(10));
    }
  }

  void Send_usingConditionVariable() const {
    bool prev = getFlag2();
    uint8_t counter = getCounter();
    while (isRunning()) {
      unique_lock lock(mtx);
      cv.wait(lock, [this]{return get_RX_IRQ() || !isRunning();});
      if (isRunning()) {
        if (prev != getFlag2()) {
          CanFrame frame{{increasedCounter(counter), 0, 0, 0, 0, 0, 0, 0}, 1, 0x200};
          
          frame.write_to_buffer(DATA_TX);

          prev = getFlag2();
          cout << "ConditionVariable | Frame Sent: ID == " << hex << "0x" << frame.id << dec << ", counter == " << getCounter() << endl;
        }

        set_TX_IRQ();
        unset_RX_IRQ();
        IRQ_CAN_SEND_MESSAGE_TX();
      }
    }
  }

  void Send_usingPolling() const {
    bool prev = getFlag2();
    uint8_t counter = getCounter();
    while (isRunning()) {
      lock_guard lock(mtx);
      if (get_RX_AVL() && isRunning()) {
        if (prev != getFlag2()) {
          CanFrame frame{{increasedCounter(counter), 0, 0, 0, 0, 0, 0, 0}, 1, 0x200};

          frame.write_to_buffer(DATA_TX);

          prev = getFlag2();
          cout << "Polling | Frame Sent: ID == " << hex << "0x" << frame.id << dec << ", counter == " << getCounter() << endl;
        }

        set_TX_AVL();
        unset_RX_AVL();
      }
    }
  }
};