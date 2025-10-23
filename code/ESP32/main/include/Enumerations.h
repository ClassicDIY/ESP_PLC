#pragma once

namespace CLASSICDIY
{
    enum NetworkSelection
    {
        NotConnected,
        APMode,
        WiFiMode,
        EthernetMode,
        ModemMode
    };

    enum NetworkState
    {
      Boot,
      ApState,
      Connecting,
      OnLine,
      OffLine
    };

    enum IOTypes  
    {
      DigitalInputs,
      AnalogInputs,
      DigitalOutputs,
      AnalogOutputs
    };
}