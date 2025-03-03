#pragma once

namespace ESP_PLC
{
    enum WiFiStatus
    {
        NotConnected,
        APMode,
        WiFiMode,

    } ;

    enum NetworkState
    {
      Boot,
      ApMode,
      Connecting,
      OnLine,
      OffLine
    };
}