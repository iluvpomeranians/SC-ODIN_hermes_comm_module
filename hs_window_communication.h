#pragma once

#include <imgui.h>
#include <chrono>
#include <thread>
#include "core/hs_ui_window.h"
#include "components/hs_component_logger.h"
#include "components/hs_component_console.h"


namespace hermes {
    class WindowCommunication : public UIWindow {

    public:
        WindowCommunication();
        ~WindowCommunication();
        void RenderGUI() override;
        string GetName() const;
        string GetPortName() const {return portName;};


        //!
        //!Connection
        //!@param portName
        //!
        //!
           HsResult Connect(const char* portName);
           HsResult Disconnect();
           bool IsConnected();

        //!
        //! Query connection Update
        //! @param elapsedTime
        //!
        void UpdateClientStatus(ComponentLogger log);

        //!
        //! For Tx/Rx byte-type conversion
        //! @param dataSize
        //!
        uint8_t ConvertDataAmount(uint64_t& dataSize);

        //!
        //!
        //!
        //! @param buffer
        //! @param nbChar
        //! @return HsResult
        //!
        int Read(char *buffer, unsigned int nbChar);

        //!
        //!
        //! @param buffer
        //! @param nbChar
        //! @return
        //!
        HsResult Write(const char *buffer, unsigned int nbChar);



    private:
        bool connected;
        HANDLE hSerial;
        COMSTAT status;
        DWORD errors;
        string portName;
        std::chrono::steady_clock::time_point m_StatsTime;       //! Time when info was collected (with history of last x values)
        std::chrono::steady_clock::time_point m_ConnectedTime ;  //! When the connection was established with this remote client
        uint64_t mStatsDataRcvdPrev;                             //!< Last amount of Bytes received since connected
        uint64_t mStatsDataSentPrev;                             //!< Last amount of Bytes sent to client since connected
        uint64_t mStatsDataSent;
        uint64_t mStatsDataRcvd;
        uint32_t mStatsSentBps;
        uint32_t mStatsRcvdBps;
        uint64_t m_rxData;
        uint64_t m_txData;



    };
}



