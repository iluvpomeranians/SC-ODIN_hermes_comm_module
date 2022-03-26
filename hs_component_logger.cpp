#include "components/hs_component_logger.h"

namespace hermes {
    ComponentLogger::ComponentLogger(string name):m_Name(std::move(name)) {
        m_AutoScroll = true;
        Clear();
    }


    void ComponentLogger::Clear() {
        m_Buf.clear();
        m_LineOffsets.clear();
        m_LineOffsets.push_back(0);
    }

    int new_size;
    void ComponentLogger::AddLog(const char* fmt, ...) IM_FMTARGS(2) {
        int old_size = m_Buf.size();
        va_list args;
        va_start(args, fmt);
        m_Buf.appendfv(fmt, args);
        va_end(args);
        for (new_size = m_Buf.size(); old_size < new_size; old_size++) {
            if (m_Buf[old_size] == '\n')
                m_LineOffsets.push_back(old_size + 1);

        }
    }

    void ComponentLogger::SaveToTextFile() {

        const char *path="C:/Users/David/Desktop/Hermes Test Text files/Logtest1.txt" ;
        ImGui::LogToClipboard();
        std::ofstream file(path);
        file << "Hermes Ground-Station Software - Space Concordia 2022 - Log Test - 1.txt\n";
        file << "Component: Logger\n\n";
        file << "Start of Log:\n--------------------------------\n\n";
        file << ImGui::GetClipboardText();


        const char *path2="C:/Users/David/Desktop/Hermes Test Text files/Logtest2.txt" ;
        ImGui::LogToClipboard();
        std::ofstream file2(path2);
        file2 << "Hermes Ground-Station Software - Space Concordia 2022 - Log Test - 2.txt\n";
        file2 << "Component: Logger\n\n";
        file2 << "Start of Log:\n--------------------------------\n\n";
        file2 << ImGui::GetClipboardText();


        const char *path3="C:/Users/David/Desktop/Hermes Test Text files/Back-up Logs/LogTest(Back-up).txt" ;
        ImGui::LogToClipboard();
        std::ofstream file3(path3);
        file3 << "Hermes Ground-Station Software - Space Concordia 2022 - Log Test - 2(Back-Up).txt\n";
        file3 << "Component: Logger\n\n";
        file3 << "Start of Log:\n--------------------------------\n\n";
        file3 << ImGui::GetClipboardText();
    }

    void ComponentLogger::RenderGUI() {
        if (!ImGui::Begin(m_Name.c_str())) {
            ImGui::End();
            return;
        }

        // Options menu
        if (ImGui::BeginPopup("Options")) {
            ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
            ImGui::EndPopup();
        }

        // Main window
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        ComponentLogger::save_log =  ImGui::Button("Save");
            if(save_log) {
                ComponentLogger::SaveToTextFile();
            }

        ImGui::SameLine();
        m_Filter.Draw("Filter", -100.0f);

        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (clear)
            Clear();
        if (copy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = m_Buf.begin();
        const char* buf_end = m_Buf.end();
        int counter = 0;
        time_t ttime = time(0);
        tm* local_time = (std::localtime(&ttime));

        if (m_Filter.IsActive()) {
            for (int line_no = 0; line_no < m_LineOffsets.Size; line_no++) {
                const char* line_start = buf + m_LineOffsets[line_no];
                const char* line_end = (line_no + 1 < m_LineOffsets.Size) ? (buf + m_LineOffsets[line_no + 1] - 1)
                                                                        : buf_end;
                if (m_Filter.PassFilter(line_start, line_end))
                    ImGui::TextUnformatted(line_start, line_end);
            }
        } else {

            ImGuiListClipper clipper;
            clipper.Begin(m_LineOffsets.Size);

            while (clipper.Step() && (m_Buf.size() >= 1)) {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
                    const char* line_start = buf + m_LineOffsets[line_no];
                    const char* line_end = (line_no + 1 < m_LineOffsets.Size) ? (buf + m_LineOffsets[line_no + 1] -
                                                                               1) : buf_end;


                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(30, 144, 255, 255));
                        ImGui::Text("[Date-%i/%i/%i] ", 1900 + local_time->tm_year, 1 + local_time->tm_mon, local_time->tm_mday);
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(153, 153, 255, 255));
                        ImGui::Text("[Time-(%i:%i:%i)]", (local_time->tm_hour), local_time->tm_min, local_time->tm_sec);
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(218, 247, 166, 255));
                        ImGui::Text(" [Log#:%i]: ", counter);
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        ImGui::TextUnformatted(line_start, line_end);
                        counter++;


                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }

    string ComponentLogger::GetName() const {
        return m_Name;
    }


}