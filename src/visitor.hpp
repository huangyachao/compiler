#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "koopa.h"
class Vistor
{
private:
    koopa_raw_program_builder_t builder;
    koopa_raw_program_t raw_program;
    std::vector<std::string> global_symbols;
    std::vector<std::string> registers = {
        "t0",
        "t1",
        "t2",
        "t3",
        "t4",
        "t5",
        "t6",
        "a0",
        "a1",
        "a2",
        "a3",
        "a4",
        "a5",
        "a6",
        "a7",
    };
    int register_count = 0;
    std::string riscv_code = "";
    std::unordered_map<koopa_raw_value_t, std::string> raw_value_map;
    std::string NewRegister()
    {
        return registers[register_count++];
    }

public:
    Vistor(std::string ir)
    {

        // 解析字符串 ir, 得到 Koopa IR 程序
        koopa_program_t program;
        koopa_error_code_t error_code = koopa_parse_from_string(ir.c_str(), &program);
        assert(error_code == KOOPA_EC_SUCCESS); // 确保解析时没有出错
        // 创建一个 raw program builder, 用来构建 raw program
        builder = koopa_new_raw_program_builder();
        // 将 Koopa IR 程序转换为 raw program
        raw_program = koopa_build_raw_program(builder, program);
        // 释放 Koopa IR 程序占用的内存
        koopa_delete_program(program);
    }

    // 访问 raw program
    void Visit()
    {
        // 访问所有全局变量
        Visit(raw_program.values);
        // 访问所有函数
        Visit(raw_program.funcs);

        std::string new_riscv_code = "  .text\n";
        for (std::string symbol : global_symbols)
        {
            new_riscv_code += "  .globl " + symbol + "\n";
        }
        riscv_code = new_riscv_code + riscv_code;
        return;
    }

    // 访问 raw slice
    void Visit(const koopa_raw_slice_t &slice)
    {
        for (size_t i = 0; i < slice.len; ++i)
        {
            auto ptr = slice.buffer[i];
            // 根据 slice 的 kind 决定将 ptr 视作何种元素
            switch (slice.kind)
            {
            case KOOPA_RSIK_FUNCTION:
                // 访问函数
                Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                // 访问基本块
                Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
                break;
            case KOOPA_RSIK_VALUE:
                // 访问指令
                Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
                break;
            default:
                // 我们暂时不会遇到其他内容, 于是不对其做任何处理
                assert(false);
            }
        }
        return;
    }

    // 访问函数
    void Visit(const koopa_raw_function_t &func)
    {
        std::string symbol = std::string(func->name).substr(1);
        global_symbols.push_back(symbol);
        riscv_code += symbol + ":\n";
        // 访问所有基本块
        Visit(func->bbs);
        return;
    }

    // 访问基本块
    void Visit(const koopa_raw_basic_block_t &bb)
    {
        // 访问所有指令
        Visit(bb->insts);
    }

    // 访问指令
    std::string Visit(const koopa_raw_value_t &value)
    {
        if (raw_value_map.count(value) > 0)
        {
            return raw_value_map[value];
        }
        std::string ret = "";
        // 根据指令类型判断后续需要如何访问
        const auto &kind = value->kind;
        switch (kind.tag)
        {
        case KOOPA_RVT_RETURN:
            // 访问 return 指令
            Visit(kind.data.ret);
            break;
        case KOOPA_RVT_INTEGER:
            // 访问 integer 指令
            ret = Visit(kind.data.integer);
            raw_value_map[value] = ret;
            break;
        case KOOPA_RVT_BINARY:
            // 访问 binary 指令
            ret = Visit(kind.data.binary);
            raw_value_map[value] = ret;
            break;

        default:
            // 其他类型暂时遇不到
            assert(false);
        }

        return ret;
    }

    std::string Visit(const koopa_raw_integer_t &value)
    {
        std::string s_val = std::to_string(value.value);
        if (s_val == "0")
        {
            return "x0";
        }
        std::string new_reg = NewRegister();
        riscv_code += "  li    " + new_reg + ", " + s_val + "\n";
        return new_reg;
    }

    void Visit(const koopa_raw_return_t &value)
    {
        koopa_raw_value_t raw_value = value.value;
        if (raw_value == nullptr)
            return;
        std::string return_value = Visit(raw_value);
        if (return_value != "")
        {
            riscv_code += "  mv    a0, " + return_value + "\n";
        }
        riscv_code += "  ret\n";
        return;
    }

    std::string Visit(const koopa_raw_binary_t &binary)
    {
        std::string first_value = Visit(binary.lhs);
        std::string second_value = Visit(binary.rhs);
        if (binary.op == KOOPA_RBO_NOT_EQ)
        {
            std::string new_register = first_value;
            if (first_value == "x0")
            {
                new_register = NewRegister();
            }
            riscv_code += "  xor   " + new_register + ", " + first_value + ", " + second_value + "\n";
            riscv_code += "  snez  " + new_register + ", " + new_register + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_EQ)
        {
            std::string new_register = first_value;
            if (first_value == "x0")
            {
                new_register = NewRegister();
            }
            riscv_code += "  xor   " + new_register + ", " + first_value + ", " + second_value + "\n";
            riscv_code += "  seqz  " + new_register + ", " + new_register + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_GT)
        {
            std::string new_register = first_value;
            if (first_value == "x0")
            {
                new_register = NewRegister();
            }
            riscv_code += "  sgt   " + new_register + ", " + first_value + ", " + second_value + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_LT)
        {
            std::string new_register = first_value;
            if (first_value == "x0")
            {
                new_register = NewRegister();
            }
            riscv_code += "  slt   " + new_register + ", " + first_value + ", " + second_value + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_GE)
        {
            std::string new_register = first_value;
            if (first_value == "x0")
            {
                new_register = NewRegister();
            }
            riscv_code += "  slt   " + new_register + ", " + first_value + ", " + second_value + "\n";
            riscv_code += "  seqz  " + new_register + ", " + new_register + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_LE)
        {
            std::string new_register = first_value;
            if (first_value == "x0")
            {
                new_register = NewRegister();
            }
            riscv_code += "  sgt   " + new_register + ", " + first_value + ", " + second_value + "\n";
            riscv_code += "  seqz  " + new_register + ", " + new_register + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_ADD)
        {
            std::string new_register = NewRegister();
            riscv_code += "  add   " + new_register + ", " + first_value + ", " + second_value + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_SUB)
        {
            std::string new_register = NewRegister();
            riscv_code += "  sub   " + new_register + ", " + first_value + ", " + second_value + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_MUL)
        {
            std::string new_register = NewRegister();
            riscv_code += "  mul   " + new_register + ", " + first_value + ", " + second_value + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_DIV)
        {
            std::string new_register = NewRegister();
            riscv_code += "  div   " + new_register + ", " + first_value + ", " + second_value + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_MOD)
        {
            std::string new_register = NewRegister();
            riscv_code += "  rem   " + new_register + ", " + first_value + ", " + second_value + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_AND)
        {
            std::string new_register = NewRegister();
            riscv_code += "  and   " + new_register + ", " + first_value + ", " + second_value + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_OR)
        {
            std::string new_register = NewRegister();
            riscv_code += "  or    " + new_register + ", " + first_value + ", " + second_value + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_XOR)
        {
            std::string new_register = NewRegister();
            riscv_code += "  xor   " + new_register + ", " + first_value + ", " + second_value + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_SHL)
        {
            std::string new_register = NewRegister();
            riscv_code += "  shl   " + new_register + ", " + first_value + ", " + second_value + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_SHL)
        {
            std::string new_register = NewRegister();
            riscv_code += "  shr   " + new_register + ", " + first_value + ", " + second_value + "\n";
            return new_register;
        }
        else if (binary.op == KOOPA_RBO_SAR)
        {
            std::string new_register = NewRegister();
            riscv_code += "  sar   " + new_register + ", " + first_value + ", " + second_value + "\n";
            return new_register;
        }

        std::string ret = "";
        abort();
        return ret;
    }

    std::string GetRiscvCode()
    {
        return riscv_code;
    }

    ~Vistor()
    {
        // 处理完成, 释放 raw program builder 占用的内存
        // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
        // 所以不要在 raw program 处理完毕之前释放 builder
        koopa_delete_raw_program_builder(builder);
    }
};