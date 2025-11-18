#pragma once
#include <vector>
#include <string>
#include "koopa.h"
class Vistor
{
private:
    koopa_raw_program_builder_t builder;
    koopa_raw_program_t raw_program;
    std::vector<std::string> global_symbols;

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
    std::string Visit()
    {
        std::string header = "  .text\n";

        std::string body = "";
        // 访问所有全局变量
        body += Visit(raw_program.values);
        // 访问所有函数
        body += Visit(raw_program.funcs);

        for (std::string symbol : global_symbols)
        {
            header += "  .globl " + symbol + "\n";
        }

        return header + body;
    }

    // 访问 raw slice
    std::string Visit(const koopa_raw_slice_t &slice)
    {
        std::string ret = "";
        for (size_t i = 0; i < slice.len; ++i)
        {
            auto ptr = slice.buffer[i];
            // 根据 slice 的 kind 决定将 ptr 视作何种元素
            switch (slice.kind)
            {
            case KOOPA_RSIK_FUNCTION:
                // 访问函数
                ret += Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                // 访问基本块
                ret += Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
                break;
            case KOOPA_RSIK_VALUE:
                // 访问指令
                ret += Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
                break;
            default:
                // 我们暂时不会遇到其他内容, 于是不对其做任何处理
                assert(false);
            }
        }
        return ret;
    }

    // 访问函数
    std::string Visit(const koopa_raw_function_t &func)
    {
        std::string symbol = std::string(func->name).substr(1);
        global_symbols.push_back(symbol);
        std::string ret = symbol + ":\n";
        // 访问所有基本块
        ret += Visit(func->bbs);
        return ret;
    }

    // 访问基本块
    std::string Visit(const koopa_raw_basic_block_t &bb)
    {
        // 访问所有指令
        return Visit(bb->insts);
    }

    // 访问指令
    std::string Visit(const koopa_raw_value_t &value)
    {
        std::string ret = "";
        // 根据指令类型判断后续需要如何访问
        const auto &kind = value->kind;
        switch (kind.tag)
        {
        case KOOPA_RVT_RETURN:
            // 访问 return 指令
            ret += Visit(kind.data.ret);
            break;
        case KOOPA_RVT_INTEGER:
            // 访问 integer 指令
            ret += Visit(kind.data.integer);
            break;
        default:
            // 其他类型暂时遇不到
            assert(false);
        }
        return ret;
    }

    std::string Visit(const koopa_raw_integer_t &value)
    {
        return std::to_string(value.value);
    }

    std::string Visit(const koopa_raw_return_t &value)
    {
        koopa_raw_value_t raw_value = value.value;
        if (raw_value == nullptr)
            return "";
        std::string ret = "  li a0, ";
        ret += Visit(raw_value) + "\n";
        ret += "  ret\n";
        return ret;
    }

    ~Vistor()
    {
        // 处理完成, 释放 raw program builder 占用的内存
        // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
        // 所以不要在 raw program 处理完毕之前释放 builder
        koopa_delete_raw_program_builder(builder);
    }
};