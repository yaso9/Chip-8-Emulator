#pragma once

#include <iostream>
#include <sstream>

#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>

#include "./Registers.hpp"

class Debugger
{
private:
    std::shared_ptr<std::array<byte, 0xFFF>> memory;
    std::shared_ptr<Registers> registers;

    MemoryEditor memory_editor;

public:
    void attach(std::shared_ptr<std::array<byte, 0xFFF>> memory, std::shared_ptr<Registers> registers)
    {
        this->memory = memory;
        this->registers = registers;
    }

    void on_clock()
    {
    }

    void draw_debugger()
    {
        ImGui::Begin("Debugger");

        if (ImGui::CollapsingHeader("Registers", ImGuiTreeNodeFlags_DefaultOpen) && ImGui::BeginTable("registers", 5))
        {
            std::ostringstream stream;
            std::string text;

            // Display the general registers
            for (uint8_t i = 0; i <= 0xF; i++)
            {
                ImGui::TableNextColumn();
                stream = std::ostringstream();
                stream << "R" << std::uppercase << std::hex << static_cast<int>(i) << ": 0x" << static_cast<int>(registers->general_regs[i]);
                text = stream.str();
                ImGui::Text(text.c_str());
            }

            // Diplay the address register
            ImGui::TableNextColumn();
            stream = std::ostringstream();
            stream << std::uppercase << std::hex << "I: 0x" << static_cast<int>(registers->addr_reg);
            text = stream.str();
            ImGui::Text(text.c_str());

            // Diplay the delay register
            ImGui::TableNextColumn();
            stream = std::ostringstream();
            stream << std::uppercase << std::hex << "DT: 0x" << static_cast<int>(registers->delay_reg);
            text = stream.str();
            ImGui::Text(text.c_str());

            // Diplay the sound register
            ImGui::TableNextColumn();
            stream = std::ostringstream();
            stream << std::uppercase << std::hex << "ST: 0x" << static_cast<int>(registers->sound_reg);
            text = stream.str();
            ImGui::Text(text.c_str());

            // Diplay the program counter
            ImGui::TableNextColumn();
            stream = std::ostringstream();
            stream << std::uppercase << std::hex << "PC: 0x" << static_cast<int>(registers->pc_reg);
            text = stream.str();
            ImGui::Text(text.c_str());

            ImGui::EndTable();
        }

        if (ImGui::CollapsingHeader("Memory", ImGuiTreeNodeFlags_DefaultOpen))
            memory_editor.DrawContents(memory.get(), memory->size());

        ImGui::End();
    }
};