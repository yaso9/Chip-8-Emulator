#pragma once

#include <iomanip>
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

    void *editing_reg;

    template <uint8_t Digits>
    constexpr const std::string get_register_format() const
    {
        std::ostringstream stream;
        stream << "%0" << static_cast<int>(Digits) << "X";
        std::string str = stream.str();
        return str;
    }

    template <uint8_t Digits, typename Type>
    void draw_hex_input(const char *label, Type *data) const
    {
        static const std::string format = get_register_format<Digits>();

        char buf[Digits + 1];
        snprintf(buf, Digits + 1, format.c_str(), *data);

        if (ImGui::InputText(label, buf, Digits + 1, ImGuiInputTextFlags_CharsHexadecimal))
        {
            sscanf(buf, format.c_str(), data);
        }
    }

    template <uint8_t Digits, typename Type>
    void draw_register(std::string name, Type *reg)
    {
        static const std::string format = get_register_format<Digits>();

        ImGui::TableNextColumn();
        ImGui::Text("%s: ", name.c_str());
        ImGui::SameLine();
        if (editing_reg != reg)
            ImGui::Text(format.c_str(), (int)*reg);
        else
        {
            draw_hex_input<Digits, Type>(("##" + name).c_str(), static_cast<Type *>(editing_reg));
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            if (ImGui::IsItemHovered())
                editing_reg = reg;
            else if (editing_reg == reg)
                editing_reg = nullptr;
    }

    void draw_registers()
    {
        if (ImGui::CollapsingHeader("Registers", ImGuiTreeNodeFlags_DefaultOpen) && ImGui::BeginTable("registers", 5))
        {
            // Display the general registers
            for (uint8_t i = 0; i <= 0xF; i++)
            {
                std::ostringstream stream;
                stream << 'R' << std::hex << std::uppercase << static_cast<int>(i);
                draw_register<2>(stream.str().c_str(), &registers->general_regs[i]);
            }

            draw_register<3>("I", &registers->addr_reg);
            draw_register<2>("DT", &registers->delay_reg);
            draw_register<2>("ST", &registers->sound_reg);
            draw_register<3>("PC", &registers->pc_reg);

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