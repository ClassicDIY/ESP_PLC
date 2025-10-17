#pragma once

namespace ESP_PLC
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
}