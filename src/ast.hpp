#pragma once
#include <memory>
#include <iostream>

class IRBuilder
{
public:
    std::string ir;
    int reg_cnt = 0;

    std::string NewReg()
    {
        return "%" + std::to_string(reg_cnt++);
    }

    void Emit(const std::string &s)
    {
        ir += s;
    }

    std::string GetIR()
    {
        return ir;
    }
};
// 所有 AST 的基类
class BaseAST
{
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
    virtual std::string GenerateIR(IRBuilder &builder) const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST
{
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override
    {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
        std::cout << " }";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        return func_def->GenerateIR(builder);
    }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override
    {
        std::cout << "FuncDefAST { ";
        func_type->Dump();
        std::cout << ", ";
        std::cout << ident << ", ";
        block->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        builder.Emit("fun @" + ident + "(): ");
        func_type->GenerateIR(builder);
        builder.Emit(" ");
        block->GenerateIR(builder);
        return "";
    }
};

class FuncTypeAST : public BaseAST
{
public:
    std::string func_type;
    void Dump() const override
    {
        std::cout << "FuncTypeAST { ";
        std::cout << func_type;
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {

        if (func_type == "int")
        {
            builder.Emit("i32");
            return "";
        }
        else
            abort();
    }
};

class BlockAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> stmt;
    void Dump() const override
    {
        std::cout << "BlockAST { ";
        stmt->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        std::string ret = "";
        builder.Emit("{\n");
        builder.Emit("%entry:\n");
        stmt->GenerateIR(builder);
        builder.Emit("}\n");
        return ret;
    }
};

class StmtAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    void Dump() const override
    {
        std::cout << "StmtAST { ";
        exp->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        std::string ret = exp->GenerateIR(builder);
        builder.Emit("  ret " + ret + "\n");
        return ret;
    }
};
class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> lor_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        lor_exp->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        return lor_exp->GenerateIR(builder);
    }
};

class MulExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> mul_exp;
    std::string op;
    std::unique_ptr<BaseAST> unar_exp;

    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            mul_exp->Dump();
            std::cout << op << " ";
        }
        unar_exp->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return unar_exp->GenerateIR(builder);
        }
        else if (op == "*")
        {
            std::string mul_exp_value = mul_exp->GenerateIR(builder);
            std::string unar_exp_value = unar_exp->GenerateIR(builder);
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = mul " + mul_exp_value + ", " + unar_exp_value + "\n");
            return reg;
        }
        else if (op == "/")
        {
            std::string mul_exp_value = mul_exp->GenerateIR(builder);
            std::string unar_exp_value = unar_exp->GenerateIR(builder);
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = div " + mul_exp_value + ", " + unar_exp_value + "\n");
            return reg;
        }
        else if (op == "%")
        {
            std::string mul_exp_value = mul_exp->GenerateIR(builder);
            std::string unar_exp_value = unar_exp->GenerateIR(builder);
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = mod " + mul_exp_value + ", " + unar_exp_value + "\n");
            return reg;
        }
        else
        {
            abort();
        }
    }
};

class AddExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> add_exp;
    std::string op;
    std::unique_ptr<BaseAST> mul_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            add_exp->Dump();
            std::cout << op << " ";
        }
        mul_exp->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return mul_exp->GenerateIR(builder);
        }
        else if (op == "+")
        {
            std::string mul_exp_value = add_exp->GenerateIR(builder);
            std::string unar_exp_value = mul_exp->GenerateIR(builder);
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = add " + mul_exp_value + ", " + unar_exp_value + "\n");
            return reg;
        }
        else if (op == "-")
        {
            std::string mul_exp_value = add_exp->GenerateIR(builder);
            std::string unar_exp_value = mul_exp->GenerateIR(builder);
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = sub " + mul_exp_value + ", " + unar_exp_value + "\n");
            return reg;
        }
        else
        {
            abort();
        }
    }
};

class RelExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> rel_exp;
    std::string op;
    std::unique_ptr<BaseAST> add_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            rel_exp->Dump();
            std::cout << op << " ";
        }
        add_exp->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return add_exp->GenerateIR(builder);
        }
        else if (op == "<")
        {
            std::string rel_exp_value = rel_exp->GenerateIR(builder);
            std::string add_exp_value = add_exp->GenerateIR(builder);
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = lt " + rel_exp_value + ", " + add_exp_value + "\n");
            return reg;
        }
        else if (op == ">")
        {
            std::string rel_exp_value = rel_exp->GenerateIR(builder);
            std::string add_exp_value = add_exp->GenerateIR(builder);
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = gt " + rel_exp_value + ", " + add_exp_value + "\n");
            return reg;
        }
        else if (op == "<=")
        {
            std::string rel_exp_value = rel_exp->GenerateIR(builder);
            std::string add_exp_value = add_exp->GenerateIR(builder);
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = le " + rel_exp_value + ", " + add_exp_value + "\n");
            return reg;
        }
        else if (op == ">=")
        {
            std::string rel_exp_value = rel_exp->GenerateIR(builder);
            std::string add_exp_value = add_exp->GenerateIR(builder);
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = ge " + rel_exp_value + ", " + add_exp_value + "\n");
            return reg;
        }
        else
        {
            abort();
        }
    }
};

class EqExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> eq_exp;
    std::string op;
    std::unique_ptr<BaseAST> rel_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            eq_exp->Dump();
            std::cout << op << " ";
        }
        rel_exp->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return rel_exp->GenerateIR(builder);
        }
        else if (op == "==")
        {
            std::string eq_exp_value = eq_exp->GenerateIR(builder);
            std::string rel_exp_value = rel_exp->GenerateIR(builder);
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = eq " + eq_exp_value + ", " + rel_exp_value + "\n");
            return reg;
        }
        else if (op == "!=")
        {
            std::string eq_exp_value = eq_exp->GenerateIR(builder);
            std::string rel_exp_value = rel_exp->GenerateIR(builder);
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = ne " + eq_exp_value + ", " + rel_exp_value + "\n");
            return reg;
        }
        else
        {
            abort();
        }
    }
};

class LAndExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> land_exp;
    std::string op;
    std::unique_ptr<BaseAST> eq_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            land_exp->Dump();
            std::cout << "&& ";
        }
        eq_exp->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return eq_exp->GenerateIR(builder);
        }
        else if (op == "&&")
        {
            std::string land_exp_value = land_exp->GenerateIR(builder);
            std::string eq_exp_value = eq_exp->GenerateIR(builder);
            std::string reg1 = builder.NewReg();
            builder.Emit("  " + reg1 + " = ne " + land_exp_value + ", " + "0" + "\n");
            std::string reg2 = builder.NewReg();
            builder.Emit("  " + reg2 + " = ne " + eq_exp_value + ", " + "0" + "\n");
            std::string reg3 = builder.NewReg();
            builder.Emit("  " + reg3 + " = and " + reg1 + ", " + reg2 + "\n");
            return reg3;
        }
        else
        {
            abort();
        }
    }
};

class LOrExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> lor_exp;
    std::string op;
    std::unique_ptr<BaseAST> land_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        if (op != "")
        {
            lor_exp->Dump();
            std::cout << "or ";
        }
        land_exp->Dump();
        std::cout << " } ";
    }

    std::string
    GenerateIR(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return land_exp->GenerateIR(builder);
        }
        else if (op == "||")
        {

            std::string lor_exp_value = lor_exp->GenerateIR(builder);
            std::string land_exp_value = land_exp->GenerateIR(builder);
            std::string reg1 = builder.NewReg();
            builder.Emit("  " + reg1 + " = ne " + lor_exp_value + ", " + "0" + "\n");
            std::string reg2 = builder.NewReg();
            builder.Emit("  " + reg2 + " = ne " + land_exp_value + ", " + "0" + "\n");
            std::string reg3 = builder.NewReg();
            builder.Emit("  " + reg3 + " = or " + reg1 + ", " + reg2 + "\n");
            return reg3;
        }
        else
        {
            abort();
        }
    }
};

class UnaryExpAST : public BaseAST
{
public:
    std::string unaryOp;
    std::unique_ptr<BaseAST> exp_ast;

    void Dump() const override
    {
        std::cout << "UnaryExp { ";
        std::cout << unaryOp;
        exp_ast->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        std::string value = exp_ast->GenerateIR(builder);
        if (unaryOp == "+")
        {
            return value;
        }
        else if (unaryOp == "-")
        {
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = sub 0, " + value + "\n");
            return reg;
        }
        else if (unaryOp == "!")
        {
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = eq " + value + ", 0" + +"\n");
            return reg;
        }
        else
        {
            if (unaryOp != "")
                abort();
            return value;
        }
    }
};

class PrimaryExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp_ast;
    void Dump() const override
    {
        std::cout << "PrimaryExp { ";
        exp_ast->Dump();
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        return exp_ast->GenerateIR(builder);
    }
};

class ParenExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp_ast;
    void Dump() const override
    {
        std::cout << "ParenExp { ";
        std::cout << " ( ";
        exp_ast->Dump();
        std::cout << " ) ";
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        return exp_ast->GenerateIR(builder);
    }
};

class NumberExpAST : public BaseAST
{
public:
    int number;
    void Dump() const override
    {
        std::cout << "NumberExp { ";
        std::cout << number;
        std::cout << " } ";
    }

    std::string GenerateIR(IRBuilder &builder) const override
    {
        return std::to_string(number);
    }
};