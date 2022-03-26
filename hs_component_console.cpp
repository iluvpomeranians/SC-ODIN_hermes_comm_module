#include "components/hs_component_console.h"

namespace hermes{


    ComponentConsole::ComponentConsole(string name):m_Name(std::move(name)) {
        ClearLog();
        memset(m_InputBuf, 0, sizeof(m_InputBuf));
        m_HistoryPos = -1;
        m_Commands.push_back("HELP");
        m_Commands.push_back("HISTORY");
        m_Commands.push_back("CLEAR");
        m_Commands.push_back("CLASSIFY");
        m_AutoScroll = true;
        m_ScrollToBottom = false;
        AddLog("<Enter Command>");
    }

    ComponentConsole::~ComponentConsole() {
        ClearLog();
        for (int i = 0; i < m_History.Size; i++)
            free(m_History[i]);
    }

    void ComponentConsole::RenderGUI() {
        if (!ImGui::BeginChild(m_Name.c_str())) {
            ImGui::EndChild();
            return;
        }

        ImGui::Separator();
        ImGui::TextWrapped("CONSOLE"); ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
        ImGui::TextWrapped("\tEnter 'HELP' for list of commands");
        ImGui::PopStyleColor();
        ImGui::Separator();


        //TODO: Possible Pop-up Save Menu to include save-as
        /*if (ImGui::Button("Save")) {
            ImGui::OpenPopup("Save");
        }
            if (ImGui::BeginPopup("Save")) {
                if (ImGui::MenuItem("Save", "Ctrl+S")) {};
                if (ImGui::MenuItem("Save As..")) {};
                ImGui::EndPopup();
            }
            */

        if (ImGui::SmallButton("Debug Text")) {
            AddLog("%d some text", m_Items.Size);
            AddLog("some more text");
            AddLog("display very important message here!");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Debug Error")) { AddLog("[error] something went wrong"); }
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear")) { ClearLog(); }
        ImGui::SameLine();
        bool copy_to_clipboard = ImGui::SmallButton("Copy");
        ImGui::SameLine();

        bool save_console = ImGui::SmallButton("Save");
        if(save_console){
            ImGui::LogToClipboard();
            const char *path="C:/Users/David/Desktop/Hermes Test Text files/ConsoleTest1.txt" ;
            std::ofstream file(path);
            file << "Hermes Ground-Station Software - Space Concordia 2022 - Console Test - 1.txt\n";
            file << "Component: Console\n\n";
            file << "Start of Console:\n--------------------------------\n\n";
            file << ImGui::GetClipboardText();

            ImGui::LogToClipboard();
            const char *path2="C:/Users/David/Desktop/Hermes Test Text files/ConsoleTest2.txt" ;
            std::ofstream file2(path2);
            file2 << "Hermes Ground-Station Software - Space Concordia 2022 - Console Test - 2.txt\n";
            file2 << "Component: Console\n\n";
            file2 << "Start of Console:\n--------------------------------\n\n";
            file2 << ImGui::GetClipboardText();

            ImGui::LogToClipboard();
            const char *path3="C:/Users/David/Desktop/Hermes Test Text files/Back-up Logs/ConsoleTest2(Back-up).txt" ;
            std::ofstream file3(path3);
            file3 << "Hermes Ground-Station Software - Space Concordia 2022 - Console Test(Back up) - 2.txt\n";
            file3 << "Component: Console\n\n";
            file3 << "Start of Console:\n--------------------------------\n\n";
            file3 << ImGui::GetClipboardText();

        }
        ImGui::SameLine();

        ImGui::Separator();

        //! Options menu
        if (ImGui::BeginPopup("Options")) {
            ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
            ImGui::EndPopup();
        }

        //! Options, m_Filter
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        m_Filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
        ImGui::Separator();

        // Reserve enough left-over height for 1 separator + 1 input text
        const float footer_height_to_reserve =
                ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
                          ImGuiWindowFlags_HorizontalScrollbar);
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::Selectable("Clear")) ClearLog();
            ImGui::EndPopup();
        }


        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
        if (copy_to_clipboard)
            ImGui::LogToClipboard();
        for (int i = 0; i < m_Items.Size; i++) {
            const char* item = m_Items[i];
            if (!m_Filter.PassFilter(item))
                continue;

            ImVec4 color;
            bool has_color = false;
            if (strstr(item, "[error]")) {
                color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                has_color = true;
            }
            else if (strncmp(item, "# ", 2) == 0) {
                color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
                has_color = true;
            }
            else if(strncmp(item, "[Hermes][Console: Port 6][Time:%i]$ %s\n", 3) == 0){
                color = ImVec4(0.0f, 1.0f, 0.4f, 1.0f);
                has_color = true;
            }
            else if(strncmp(item, "Warning - Unknown Command: '%s'\n", 4) == 0){
                color = ImVec4(0.9f, 0.9f, 0.0f, 0.5f);
                has_color = true;
            }
            if (has_color)
                ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(item);
            if (has_color)
                ImGui::PopStyleColor();
        }
        if (copy_to_clipboard)
            ImGui::LogFinish();

        if (m_ScrollToBottom || (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            ImGui::SetScrollHereY(1.0f);
        m_ScrollToBottom = false;

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::Separator();

        //! Command-line
        bool reclaim_focus = false;
        ImGuiInputTextFlags input_text_flags =
                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion |
                ImGuiInputTextFlags_CallbackHistory;
        if (ImGui::InputText("Input", m_InputBuf, IM_ARRAYSIZE(m_InputBuf), input_text_flags, &TextEditCallbackStub,
                             (void*) this)) {
            char* s = m_InputBuf;
            Strtrim(s);
            if (s[0])
                ExecCommand(s);
            strcpy(s, "");
            reclaim_focus = true;

        }

        //! Auto-focus on window apparition
        ImGui::SetItemDefaultFocus();
        if (reclaim_focus)
            ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

        ImGui::EndChild();
    }

    int ComponentConsole::Stricmp(const char* s1, const char* s2) {
        int d;
        while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) {
            s1++;
            s2++;
        }
        return d;
    }

    int ComponentConsole::Strnicmp(const char* s1, const char* s2, int n) {
        int d = 0;
        while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) {
            s1++;
            s2++;
            n--;
        }
        return d;
    }

    char* ComponentConsole::Strdup(const char* s) {
        IM_ASSERT(s);
        size_t len = strlen(s) + 1;
        void* buf = malloc(len);
        IM_ASSERT(buf);
        return (char*) memcpy(buf, (const void*) s, len);
    }

    void ComponentConsole::Strtrim(char* s) {
        char* str_end = s + strlen(s);
        while (str_end > s && str_end[-1] == ' ') str_end--;
        *str_end = 0;
    }

    void ComponentConsole::ClearLog() {
        for (int i = 0; i < m_Items.Size; i++)
            free(m_Items[i]);
        m_Items.clear();
    }

    void ComponentConsole::AddLog(const char* fmt, ...) IM_FMTARGS(2) {
            // FIXME-OPT
            char buf[1024];
            va_list args;
            va_start(args, fmt);
            vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
            buf[IM_ARRAYSIZE(buf) - 1] = 0;
            va_end(args);
            m_Items.push_back(Strdup(buf));
    }

    void ComponentConsole::ExecCommand(const char* command_line) {
        AddLog("[Hermes][Console: Port 6][Time:%i]$ %s\n", ImPlot::GetStyle().UseISO8601, command_line);


        //!History Callback
        m_HistoryPos = -1;
        for (int i = m_History.Size - 1; i >= 0; i--)
            if (Stricmp(m_History[i], command_line) == 0) {
                free(m_History[i]);
                m_History.erase(m_History.begin() + i);
                break;
            }
        m_History.push_back(Strdup(command_line));

        // Process command
        if (Stricmp(command_line, "CLEAR") == 0) {
            ClearLog();
        } else if (Stricmp(command_line, "HELP") == 0) {
            AddLog("m_Commands:");
            for (int i = 0; i < m_Commands.Size; i++)
                AddLog("- %s", m_Commands[i]);
        } else if (Stricmp(command_line, "HISTORY") == 0) {
            int first = m_History.Size - 10;
            for (int i = first > 0 ? first : 0; i < m_History.Size; i++)
                AddLog("%3d: %s\n", i, m_History[i]);
        } else {
            AddLog("Warning - Unknown Command: '%s'\n", command_line);
        }

        m_ScrollToBottom = true;
    }

     int ComponentConsole::TextEditCallbackStub(ImGuiInputTextCallbackData* data) {
        ComponentConsole* console = (ComponentConsole*) data->UserData;
        return console->TextEditCallback(data);
    }

    int ComponentConsole::TextEditCallback(ImGuiInputTextCallbackData* data) {
        switch (data->EventFlag) {
            case ImGuiInputTextFlags_CallbackCompletion: {

                const char* word_end = data->Buf + data->CursorPos;
                const char* word_start = word_end;
                while (word_start > data->Buf) {
                    const char c = word_start[-1];
                    if (c == ' ' || c == '\t' || c == ',' || c == ';')
                        break;
                    word_start--;
                }

                ImVector<const char*> candidates;
                for (int i = 0; i < m_Commands.Size; i++)
                    if (Strnicmp(m_Commands[i], word_start, (int) (word_end - word_start)) == 0)
                        candidates.push_back(m_Commands[i]);

                if (candidates.Size == 0) {
                    // No match
                    AddLog("No match for \"%.*s\"!\n", (int) (word_end - word_start), word_start);
                } else if (candidates.Size == 1) {
                    data->DeleteChars((int) (word_start - data->Buf), (int) (word_end - word_start));
                    data->InsertChars(data->CursorPos, candidates[0]);
                    data->InsertChars(data->CursorPos, " ");
                } else {
                    int match_len = (int) (word_end - word_start);
                    for (;;) {
                        int c = 0;
                        bool all_candidates_matches = true;
                        for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                            if (i == 0)
                                c = toupper(candidates[i][match_len]);
                            else if (c == 0 || c != toupper(candidates[i][match_len]))
                                all_candidates_matches = false;
                        if (!all_candidates_matches)
                            break;
                        match_len++;
                    }

                    if (match_len > 0) {
                        data->DeleteChars((int) (word_start - data->Buf), (int) (word_end - word_start));
                        data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                    }

                    AddLog("Possible matches:\n");
                    for (int i = 0; i < candidates.Size; i++)
                        AddLog("- %s\n", candidates[i]);
                }

                break;
            }
            case ImGuiInputTextFlags_CallbackHistory: {
                // Example of HISTORY
                const int prev_history_pos = m_HistoryPos;
                if (data->EventKey == ImGuiKey_UpArrow) {
                    if (m_HistoryPos == -1)
                        m_HistoryPos = m_History.Size - 1;
                    else if (m_HistoryPos > 0)
                        m_HistoryPos--;
                } else if (data->EventKey == ImGuiKey_DownArrow) {
                    if (m_HistoryPos != -1)
                        if (++m_HistoryPos >= m_History.Size)
                            m_HistoryPos = -1;
                }

                if (prev_history_pos != m_HistoryPos) {
                    const char* history_str = (m_HistoryPos >= 0) ? m_History[m_HistoryPos] : "";
                    data->DeleteChars(0, data->BufTextLen);
                    data->InsertChars(0, history_str);
                }
            }
        }
        return 0;
    }

    string ComponentConsole::GetName() const {
        return m_Name;
    }
}