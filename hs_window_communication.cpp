#include "windows/hs_window_communication.h"

namespace hermes {

    WindowCommunication::WindowCommunication() {
        mStatsDataRcvdPrev = 0;                           //! Last amount of Bytes received since connected
        mStatsDataSentPrev = 0;                           //! Last amount of Bytes sent to client since connected
        mStatsDataSent = 0;                               //! Current amount of Bytes sent to client since connected
        mStatsRcvdBps = 0;                                //! Average Bytes received per second
        mStatsSentBps = 0;                                //! Average Bytes sent per second
        this->connected = false; }

    WindowCommunication::~WindowCommunication() {}

    HsResult WindowCommunication::Connect(const char* portName) {

            //Try to connect to the given port through CreateFile
            this->portName = portName;
            this->hSerial = CreateFile(portName,
                                       GENERIC_READ | GENERIC_WRITE,
                                       0,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL);

            printf("Connection Initialized\n");

            //Check if the connection was successful
            if (this->hSerial == INVALID_HANDLE_VALUE) {
                //If not successful display an Error
                if (GetLastError() == ERROR_FILE_NOT_FOUND) {

                    //Print Error if necessary
                    printf("ERROR: Handle was not attached. Reason: %s not available.\n", portName);

                } else {
                    printf("ERROR!!!");
                }
            } else {
                //If connected we try to set the comm parameters
                DCB dcbSerialParams = {0};


                //Try to get the current
                if (!GetCommState(this->hSerial, &dcbSerialParams)) {
                    //If impossible, show an error
                    printf("failed to get current serial parameters!");
                } else {
                    //Define serial connection parameters for the arduino board
                    dcbSerialParams.BaudRate = CBR_9600;
                    dcbSerialParams.ByteSize = 8;
                    dcbSerialParams.StopBits = ONESTOPBIT;
                    dcbSerialParams.Parity = NOPARITY;
                    //Setting the DTR to Control_Enable ensures that the Arduino is properly
                    //reset upon establishing a connection
                    dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;


                    //Set the parameters and check for their proper application
                    if (!SetCommState(hSerial, &dcbSerialParams)) {
                        printf("ALERT: Could not set Serial Port parameters");
                    } else {
                        //If everything went fine we're connected
                        this->connected = true;
                        m_StatsTime = std::chrono::steady_clock::now();
                        m_ConnectedTime = std::chrono::steady_clock::now();
                        //Flush any remaining characters in the buffers
                        PurgeComm(this->hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
                        //We wait 2s as the arduino board will be reseting
                        Sleep(2000);
                    }
                }
            }

            return HS_SUCCESS;
    }

    HsResult WindowCommunication::Disconnect() {
        //Check if we are connected before trying to disconnect
        if (this->connected) {
            //We're no longer connected
            this->connected = false;
            //Close the serial handler
            CloseHandle(this->hSerial);
            printf("Connection Terminated\n");
        }
        return HS_SUCCESS;
    }

    bool WindowCommunication::IsConnected() {
        //Simply return the connection status
        return this->connected;
    }


    const char*	kDataSizeUnits[] = {" B", " KB", " MB", " GB"};
    uint8_t WindowCommunication::ConvertDataAmount(uint64_t &dataSize) {

            uint8_t outUnitIdx	= 0;
            for(size_t i(0); i<IM_ARRAYSIZE(kDataSizeUnits); ++i)
            {
                outUnitIdx	+= dataSize >= 1024 * 100 ? 1 : 0;
                dataSize	/= outUnitIdx > i ? 1024 : 1;
            }
            return outUnitIdx;
    }

    static std::atomic_uint32_t gStatsDataRcvd(0);
    static std::atomic_uint32_t gStatsDataSent(0);
    void WindowCommunication::UpdateClientStatus(ComponentLogger log) {

                auto elapsedTime = std::chrono::steady_clock::now() - m_StatsTime;       //! STATS TIMER
                auto SessionTime = std::chrono::steady_clock::now() - m_ConnectedTime;   //! SESSION TIMER
                int tmSec = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(SessionTime).count() %60);
                int tmMin = static_cast<int>(std::chrono::duration_cast<std::chrono::minutes>(SessionTime).count() %60);
                int tmHour = static_cast<int>(std::chrono::duration_cast<std::chrono::hours>(SessionTime).count());


                if( std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count() >= 250 ) {
                    mStatsDataRcvd = log.GetBufferSize();
                    constexpr uint64_t kHysteresis = 10; // out of 100
                    uint64_t newDataRcvd = mStatsDataRcvd - mStatsDataRcvdPrev;
                    //std::cout << mStatsDataRcvd << " - " << mStatsDataRcvdPrev << std::endl; - Debug to check (current - previous) values
                    uint64_t newDataSent = mStatsDataSent - mStatsDataSentPrev;
                    uint64_t tmMicrosS = std::chrono::duration_cast<std::chrono::microseconds>(elapsedTime).count(); //! Conversion duration cast to Microseconds
                    uint32_t newDataRcvdBps = static_cast<uint32_t>(newDataRcvd * 1000000u / tmMicrosS);
                    uint32_t newDataSentBps = static_cast<uint32_t>(newDataSent * 1000000u / tmMicrosS);
                    mStatsRcvdBps = (mStatsRcvdBps * (100u - kHysteresis) + newDataRcvdBps * kHysteresis) / 100u;
                    mStatsSentBps = (mStatsSentBps * (100u - kHysteresis) + newDataSentBps * kHysteresis) / 100u;
                    gStatsDataRcvd += newDataRcvd;
                    gStatsDataSent += newDataSent;

                    m_StatsTime = std::chrono::steady_clock::now();
                    mStatsDataRcvdPrev = mStatsDataRcvd;
                    mStatsDataSentPrev = mStatsDataSent;

                }
                m_rxData = mStatsDataRcvdPrev;
                m_txData = mStatsDataSentPrev;
                uint8_t txUnitIdx = ConvertDataAmount(m_txData);
                uint8_t rxUnitIdx = ConvertDataAmount(m_rxData);


                ImGui::TextUnformatted("Session Time");
                ImGui::SameLine();
                ImGui::Text(": %03ih%02i:%02i", tmHour, tmMin, tmSec);
                ImGui::TextUnformatted("Data :");
                ImGui::Text(" \t: (Rx) %7.2f  KB/s \t(Tx) %7.2f  KB/s", static_cast<float>(mStatsRcvdBps)/1000, static_cast<float>(mStatsSentBps)/1000);
                ImGui::Text(" \t: (Rx) %7i  B/s  \t(Tx) %7i  B/s", mStatsRcvdBps , mStatsSentBps);
                ImGui::Text(" \t: (Rx) %7i  b/s  \t(Tx) %7i  b/s", mStatsRcvdBps*8 , mStatsSentBps*8);
                ImGui::Text(" \t: (Rx) %7i %s    \t(Tx) %7i %s", static_cast<int>(m_rxData), kDataSizeUnits[rxUnitIdx], static_cast<int>(m_txData), kDataSizeUnits[txUnitIdx]);


                //!Test Real-Time Scrolling Buffer Graphs
                //TODO: ORGANIZE/CREATE NEW FUNCTION

                float ConvertedFloatRcvdBpsRate = static_cast<float>(mStatsRcvdBps);
                float ConvertedFloatRcvdBitpsRate = static_cast<float>(mStatsRcvdBps*8);

                struct ScrollingBuffer {
                    int MaxSize;
                    int Offset;
                    ImVector<ImVec2> Data;
                    ScrollingBuffer(int max_size = 2000) {
                        MaxSize = max_size;
                        Offset  = 0;
                        Data.reserve(MaxSize);
                    }
                    void AddPoint(float x, float y) {
                        if (Data.size() < MaxSize)
                            Data.push_back(ImVec2(x,y));
                        else {
                            Data[Offset] = ImVec2(x,y);
                            Offset =  (Offset + 1) % MaxSize;
                        }
                    }
                    void Erase() {
                        if (Data.size() > 0) {
                            Data.shrink(0);
                            Offset  = 0;
                        }
                    }
                };
                static ScrollingBuffer sdata1, sdata2;
                static float t = 0;
                t += ImGui::GetIO().DeltaTime;
                sdata1.AddPoint(t, ConvertedFloatRcvdBpsRate);
                sdata2.AddPoint(t, ConvertedFloatRcvdBitpsRate);
                static float history = 3.0f;
                ImGui::SliderFloat("History",&history,1,30,"%.1f s");
                static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;
                ImPlot::ShowColormapSelector("Color");


                if (ImPlot::BeginPlot("##Scrolling", ImVec2(1248,115))) {
                    ImPlot::SetupAxes(NULL, NULL, flags, flags);
                    ImPlot::SetupAxisLimits(ImAxis_X1,t - history, t, ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1,0,9600);
                    ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL,0.9f);
                    //ImPlot::PlotShaded("Speed B/s", &sdata1.Data[0].x, &sdata1.Data[0].y, sdata1.Data.size(), -INFINITY, sdata1.Offset, 2 * sizeof(float));
                    ImPlot::PlotLine("Speed B/s", &sdata1.Data[0].x, &sdata1.Data[0].y, sdata1.Data.size(), sdata1.Offset, 2*sizeof(float));
                    ImPlot::PlotLine("Speed b/s", &sdata2.Data[0].x, &sdata2.Data[0].y, sdata2.Data.size(), sdata2.Offset, 2*sizeof(float));
                    ImPlot::EndPlot();
        }


    }



    int WindowCommunication::Read(char* buffer, unsigned int nbChar) {
        DWORD bytesRead;
        unsigned int toRead;

        ClearCommError(this->hSerial, &this->errors, &this->status);

        if (this->status.cbInQue > 0) {
            if (this->status.cbInQue > nbChar) {
                toRead = nbChar;
            } else {
                toRead = this->status.cbInQue;
            }

            if (ReadFile(this->hSerial, buffer, toRead, &bytesRead, NULL)) {
                return bytesRead;
            }
        }

        return 0;
    }


    HsResult WindowCommunication::Write(const char* buffer, unsigned int nbChar) {
        DWORD bytesSend;
        if (!WriteFile(this->hSerial, (void*) buffer, nbChar, &bytesSend, 0)) {
            ClearCommError(this->hSerial, &this->errors, &this->status);

            return HS_ERROR;
        } else {
            return HS_SUCCESS;
        }
    }

    string WindowCommunication::GetName() const {
        return "Communication Terminal";
    }

    void WindowCommunication::RenderGUI() {
        ImGui::StyleColorsClassic();
        //!CREATE & RENDER LOG

        static ComponentLogger log("Logger");
        log.RenderGUI();
        ImGui::Begin("Connection");

        //!COMBO LIST

        const char* items[] = { "COM 1", "COM 6", "COM 7", };
        static int item_current_idx = 0;
        const char* combo_preview_value = items[item_current_idx];
        if (ImGui::BeginCombo("COMM PORT SELECT", combo_preview_value))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                const bool is_selected = (item_current_idx == n);
                if (ImGui::Selectable(items[n], is_selected))
                    item_current_idx = n;
                if(IsConnected()){
                    Disconnect();
                }

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        //!BUTTONS

                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) ImColor::HSV(2 / 7.0f, 0.6f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4) ImColor::HSV(2 / 7.0f, 0.7f, 0.7f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4) ImColor::HSV(2 / 7.0f, 0.8f, 0.8f));
                if (ImGui::Button("Connect")) {

                    if(IsConnected()){
                    }
                    else {
                        if (item_current_idx == 0) {
                            Connect("\\\\.\\COM1");
                        }
                        if (item_current_idx == 1) {
                            Connect("\\\\.\\COM6");
                            }

                        if (item_current_idx == 2) {
                            Disconnect();
                        }
                        if (IsConnected()) {
                            printf("Connection Established\n");
                        }
                    }
                }
                ImGui::PopStyleColor(3);
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) ImColor::HSV(0 / 7.0f, 0.6f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4) ImColor::HSV(0 / 7.0f, 0.7f, 0.7f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4) ImColor::HSV(0 / 7.0f, 0.8f, 0.8f));
                if (ImGui::Button("Disconnect")) {
                    Disconnect();
                };
                ImGui::PopStyleColor(3);
                ImGui::SameLine();



        //!SET-UP LOG & Update Client Status

                if (IsConnected() && item_current_idx == 1) {
                    std::time_t t = std::time(0);
                    std::tm* now = std::localtime(&t);
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
                    ImGui::Text("Connection Established");
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 255, 255));
                    ImGui::Text(" - Current Port: COM# 6");
                    ImGui::PopStyleColor();
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
                    ImGui::Text("Date : %i-%i-%i", 1900 + now->tm_year, 1 + now->tm_mon, now->tm_mday);
                    ImGui::PopStyleColor();
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
                    ImGui::Text("Local Time : %i:%i:%i EST", now->tm_hour, 1 + now->tm_min, now->tm_sec);
                    ImGui::PopStyleColor();
                    char incomingData[256] = "";
                    int dataLength = 255;
                    int readResult = 0;
                    readResult = WindowCommunication::Read(incomingData, dataLength);
                    incomingData[readResult] = 0;
                    log.AddLog("%s", incomingData);
                    WindowCommunication::UpdateClientStatus(log);

                }
                else if (IsConnected() && item_current_idx == 0) {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
                        ImGui::Text("Connection Established");
                        ImGui::PopStyleColor();
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 255, 255));
                        ImGui::Text("Current Port: COM# 1");
                        ImGui::PopStyleColor();
                        char incomingData[256] = "";            // don't forget to pre-allocate memory
                        //printf("%s\n",incomingData);
                        int dataLength = 255;
                        int readResult = 0;
                        readResult = WindowCommunication::Read(incomingData, dataLength);
                        // printf("Bytes read: (0 means no data available) %i\n",readResult);
                        incomingData[readResult] = 0;
                        log.AddLog("%s", incomingData);
                        WindowCommunication::UpdateClientStatus(log);

                }
                else if (IsConnected() && item_current_idx == 2) {

                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
                        ImGui::SameLine();ImGui::Text("- COM PORT 7 NOT AVAILABLE");
                        ImGui::PopStyleColor();


                }else {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                    ImGui::Text("Disconnected");
                    ImGui::PopStyleColor();
                    mStatsDataRcvd = 0;
                    mStatsDataSent = 0;
                    mStatsSentBps = 0;
                    mStatsRcvdBps = 0;
                    mStatsDataRcvdPrev = 0;
                    mStatsDataSentPrev = 0;
                    m_rxData = 0;
                    m_txData = 0;
                    log.Clear();
                }

        //!CREATE & RENDER CONSOLE

                ImGui::Separator();
                static ComponentConsole console("Console");
                console.RenderGUI();


        ImGui::End();

    }

}

