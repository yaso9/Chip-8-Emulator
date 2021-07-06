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

    void draw_registers()
    {
        if (ImGui::CollapsingHeader("Registers", ImGuiTreeNodeFlags_DefaultOpen) && ImGui::BeginTable("registers", 5))
        {
            // Display the general registers
            for (uint8_t i = 0; i <= 0xF; i++)
            {
                ImGui::TableNextColumn();
                ImGui::Text("R%X: 0x%02X", i, registers->general_regs[i]);
            }

            // Display the address register
            ImGui::TableNextColumn();
            ImGui::Text("I: 0x%03X", registers->addr_reg);

            // Display the delay register
            ImGui::TableNextColumn();
            ImGui::Text("DT: 0x%02X", registers->delay_reg);

            // Display the sound register
            ImGui::TableNextColumn();
            ImGui::Text("ST: 0x%02X", registers->sound_reg);

            // Display the program counter
            ImGui::TableNextColumn();
            ImGui::Text("PC: 0x%03X", registers->pc_reg);

            ImGui::EndTable();
        }
    }

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

        draw_registers();

        if (ImGui::CollapsingHeader("Memory", ImGuiTreeNodeFlags_DefaultOpen))
            memory_editor.DrawContents(memory.get(), memory->size());

        ImGui::End();
    }
};