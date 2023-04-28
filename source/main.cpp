#include <CanDriver.h>

int main() {
  CanDriver Driver;

  {
    Driver.start(CanDriver::Mode::Polling);
    thread ReadThread(&CanDriver::DataReceive, &Driver);
    thread SendThread(&CanDriver::DataSend   , &Driver);

    cin.get();
    Driver.stop();

    ReadThread.join();
    SendThread.join();
  }

  {
    Driver.start(CanDriver::Mode::ConditionVariable);
    thread ReadThread(&CanDriver::DataReceive, &Driver);
    thread SendThread(&CanDriver::DataSend   , &Driver);

    cin.get();
    Driver.stop();
    cv.notify_one();

    ReadThread.join();
    SendThread.join();
  }

  return 0;
}