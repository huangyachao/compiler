#pragma once
#include <memory>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <variant>

class SymbolTable
{
private:
    std::unordered_map<std::string, std::string> table;

public:
    void Insert(std::string name, std::string value)
    {
        table[name] = value;
    }

    std::string Get(std::string name)
    {
        return table[name];
    }
};

extern SymbolTable global_symbol_table;

enum class Type
{
    Int,
    Reigster,
    Variable,
    String,
    Unknown,
};

class IRValue
{
private:
    Type type;
    // std::variant<int, std::string> data;
    std::string data;

public:
    IRValue(Type type, std::string data)
    {
        this->type = type;
        this->data = data;
    }

    std::string ToString()
    {
        return data;
    }
    bool IsConst()
    {
        return type == Type::Int;
    }

    int GetConstValue()
    {
        return std::stoi(data);
    }
};

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
    virtual IRValue GenerateIRExpr(IRBuilder &) const { return IRValue(Type::Unknown, ""); }
    virtual void GenerateIRStmt(IRBuilder &) const {}
};

class BaseExpAST : public BaseAST
{
public:
    virtual ~BaseExpAST() = default;
    virtual void Dump() const = 0;
    virtual IRValue GenerateIRExpr(IRBuilder &) const = 0;
};

class BaseStmtAST : public BaseAST
{
public:
    virtual ~BaseStmtAST() = default;
    virtual void Dump() const = 0;
    virtual void GenerateIRStmt(IRBuilder &) const = 0;
};

// CompUnit 是 BaseStmtAST
class CompUnitAST : public BaseStmtAST
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

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        func_def->GenerateIRStmt(builder);
    }
};

// FuncDef 也是 BaseStmtAST
class FuncDefAST : public BaseStmtAST
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

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        builder.Emit("fun @" + ident + "(): ");
        func_type->GenerateIRStmt(builder);
        builder.Emit(" ");
        block->GenerateIRStmt(builder);
    }
};

class FuncTypeAST : public BaseStmtAST
{
public:
    std::string func_type;
    void Dump() const override
    {
        std::cout << "FuncTypeAST { ";
        std::cout << func_type;
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {

        if (func_type == "int")
        {
            builder.Emit("i32");
        }
        else
            abort();
    }
};

class BlockAST : public BaseStmtAST
{
public:
    std::vector<std::unique_ptr<BaseAST>> block_items;
    void Dump() const override
    {
        std::cout << "BlockAST { ";
        for (const auto &item : block_items)
        {
            item->Dump();
        }
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        builder.Emit("{\n");
        builder.Emit("%entry:\n");
        for (const auto &item : block_items)
        {
            item->GenerateIRStmt(builder);
        }
        builder.Emit("}\n");
    }
};

class BlockItemAST : public BaseStmtAST
{
public:
    std::unique_ptr<BaseAST> item;
    void Dump() const override
    {
        std::cout << "BlockItemAST { ";
        item->Dump();
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        item->GenerateIRStmt(builder);
    }
};

class DeclAST : public BaseStmtAST
{
public:
    std::unique_ptr<BaseAST> const_decl;
    void Dump() const override
    {
        std::cout << "DeclAST { ";
        const_decl->Dump();
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        const_decl->GenerateIRStmt(builder);
    }
};

class ConstDeclAST : public BaseStmtAST
{
public:
    std::unique_ptr<BaseAST> btype;
    std::vector<std::unique_ptr<BaseAST>> const_defs;
    void Dump() const override
    {
        std::cout << "DeclAST { const ";
        btype->Dump();
        for (const auto &const_def : const_defs)
        {
            const_def->Dump();
        }
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        btype->GenerateIRStmt(builder);
        for (const auto &const_def : const_defs)
        {
            const_def->GenerateIRStmt(builder);
        }
    }
};

class BTypeAST : public BaseExpAST
{
public:
    std::string btype;
    void Dump() const override
    {
        std::cout << "BTypeAST { ";
        std::cout << btype;
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return IRValue(Type::String, btype);
    }
};

class ConstDefAST : public BaseStmtAST
{
public:
    std::string ident;
    std::unique_ptr<BaseAST> const_init_val;
    void Dump() const override
    {
        std::cout << "ConstDefAST { ";
        std::cout << ident;
        std::cout << " = ";
        const_init_val->Dump();
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        std::string value = const_init_val->GenerateIRExpr(builder).ToString();
        global_symbol_table.Insert(ident, value);
    }
};

class ConstInitValAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> const_exp;
    void Dump() const override
    {
        std::cout << "ConstInitValAST { ";
        const_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return const_exp->GenerateIRExpr(builder);
    }
};

class ConstExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> exp;
    void Dump() const override
    {
        std::cout << "ConstExpAST { ";
        exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return exp->GenerateIRExpr(builder);
    }
};

class LValAST : public BaseExpAST
{
public:
    std::string ident;
    void Dump() const override
    {
        std::cout << "LValAST { ";
        std::cout << ident;
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        std::string value = global_symbol_table.Get(ident);
        return IRValue(Type::Int, value);
    }
};
class StmtAST : public BaseStmtAST
{
public:
    std::unique_ptr<BaseAST> exp;
    void Dump() const override
    {
        std::cout << "StmtAST { ";
        exp->Dump();
        std::cout << " } ";
    }

    void GenerateIRStmt(IRBuilder &builder) const override
    {
        std::string ret = exp->GenerateIRExpr(builder).ToString();
        builder.Emit("  ret " + ret + "\n");
    }
};
class ExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> lor_exp;
    void Dump() const override
    {
        std::cout << "ExpAST { ";
        lor_exp->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return lor_exp->GenerateIRExpr(builder);
    }
};

class MulExpAST : public BaseExpAST
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

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return unar_exp->GenerateIRExpr(builder);
        }
        else if (op == "*")
        {
            IRValue mul_exp_value = mul_exp->GenerateIRExpr(builder);
            IRValue unar_exp_value = unar_exp->GenerateIRExpr(builder);
            if (mul_exp_value.IsConst() && unar_exp_value.IsConst())
            {
                int result = mul_exp_value.GetConstValue() * unar_exp_value.GetConstValue();
                return IRValue(Type::Int, std::to_string(result));
            }
            else
            {
                std::string reg = builder.NewReg();
                builder.Emit("  " + reg + " = mul " + mul_exp_value.ToString() + ", " + unar_exp_value.ToString() + "\n");
                return IRValue(Type::Reigster, reg);
            }
        }
        else if (op == "/")
        {
            IRValue mul_exp_value = mul_exp->GenerateIRExpr(builder);
            IRValue unar_exp_value = unar_exp->GenerateIRExpr(builder);
            if (mul_exp_value.IsConst() && unar_exp_value.IsConst())
            {
                int result = mul_exp_value.GetConstValue() / unar_exp_value.GetConstValue();
                return IRValue(Type::Int, std::to_string(result));
            }
            else
            {
                std::string reg = builder.NewReg();
                builder.Emit("  " + reg + " = div " + mul_exp_value.ToString() + ", " + unar_exp_value.ToString() + "\n");
                return IRValue(Type::Reigster, reg);
            }
        }
        else if (op == "%")
        {
            IRValue mul_exp_value = mul_exp->GenerateIRExpr(builder);
            IRValue unar_exp_value = unar_exp->GenerateIRExpr(builder);
            if (mul_exp_value.IsConst() && unar_exp_value.IsConst())
            {
                int result = mul_exp_value.GetConstValue() % unar_exp_value.GetConstValue();
                return IRValue(Type::Int, std::to_string(result));
            }
            else
            {
                std::string reg = builder.NewReg();
                builder.Emit("  " + reg + " = mod " + mul_exp_value.ToString() + ", " + unar_exp_value.ToString() + "\n");
                return IRValue(Type::Reigster, reg);
            }
        }
        else
        {
            abort();
        }
    }
};

class AddExpAST : public BaseExpAST
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

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return mul_exp->GenerateIRExpr(builder);
        }
        else if (op == "+")
        {
            IRValue add_exp_value = add_exp->GenerateIRExpr(builder);
            IRValue mul_exp_value = mul_exp->GenerateIRExpr(builder);
            if (add_exp_value.IsConst() && mul_exp_value.IsConst())
            {
                int result = add_exp_value.GetConstValue() + mul_exp_value.GetConstValue();
                return IRValue(Type::Int, std::to_string(result));
            }
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = add " + add_exp_value.ToString() + ", " + mul_exp_value.ToString() + "\n");
            return IRValue(Type::Reigster, reg);
        }
        else if (op == "-")
        {
            IRValue add_exp_value = add_exp->GenerateIRExpr(builder);
            IRValue mul_exp_value = mul_exp->GenerateIRExpr(builder);
            if (add_exp_value.IsConst() && mul_exp_value.IsConst())
            {
                int result = add_exp_value.GetConstValue() - mul_exp_value.GetConstValue();
                return IRValue(Type::Int, std::to_string(result));
            }
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = sub " + add_exp_value.ToString() + ", " + mul_exp_value.ToString() + "\n");
            return IRValue(Type::Reigster, reg);
        }
        else
        {
            abort();
        }
    }
};

class RelExpAST : public BaseExpAST
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

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return add_exp->GenerateIRExpr(builder);
        }
        else if (op == "<")
        {
            IRValue rel_exp_value = rel_exp->GenerateIRExpr(builder);
            IRValue add_exp_value = add_exp->GenerateIRExpr(builder);
            if (rel_exp_value.IsConst() && add_exp_value.IsConst())
            {
                int result = rel_exp_value.GetConstValue() < add_exp_value.GetConstValue();
                return IRValue(Type::Int, std::to_string(result));
            }
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = lt " + rel_exp_value.ToString() + ", " + add_exp_value.ToString() + "\n");
            return IRValue(Type::Reigster, reg);
        }
        else if (op == ">")
        {
            IRValue rel_exp_value = rel_exp->GenerateIRExpr(builder);
            IRValue add_exp_value = add_exp->GenerateIRExpr(builder);
            if (rel_exp_value.IsConst() && add_exp_value.IsConst())
            {
                int result = rel_exp_value.GetConstValue() > add_exp_value.GetConstValue();
                return IRValue(Type::Int, std::to_string(result));
            }
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = gt " + rel_exp_value.ToString() + ", " + add_exp_value.ToString() + "\n");
            return IRValue(Type::Reigster, reg);
        }
        else if (op == "<=")
        {
            IRValue rel_exp_value = rel_exp->GenerateIRExpr(builder);
            IRValue add_exp_value = add_exp->GenerateIRExpr(builder);
            if (rel_exp_value.IsConst() && add_exp_value.IsConst())
            {
                int result = rel_exp_value.GetConstValue() <= add_exp_value.GetConstValue();
                return IRValue(Type::Int, std::to_string(result));
            }
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = le " + rel_exp_value.ToString() + ", " + add_exp_value.ToString() + "\n");
            return IRValue(Type::Reigster, reg);
        }
        else if (op == ">=")
        {
            IRValue rel_exp_value = rel_exp->GenerateIRExpr(builder);
            IRValue add_exp_value = add_exp->GenerateIRExpr(builder);
            if (rel_exp_value.IsConst() && add_exp_value.IsConst())
            {
                int result = rel_exp_value.GetConstValue() >= add_exp_value.GetConstValue();
                return IRValue(Type::Int, std::to_string(result));
            }
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = ge " + rel_exp_value.ToString() + ", " + add_exp_value.ToString() + "\n");
            return IRValue(Type::Reigster, reg);
        }
        else
        {
            abort();
        }
    }
};

class EqExpAST : public BaseExpAST
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

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return rel_exp->GenerateIRExpr(builder);
        }
        else if (op == "==")
        {

            IRValue eq_exp_value = eq_exp->GenerateIRExpr(builder);
            IRValue rel_exp_value = rel_exp->GenerateIRExpr(builder);
            if (eq_exp_value.IsConst() && rel_exp_value.IsConst())
            {
                int result = eq_exp_value.GetConstValue() == rel_exp_value.GetConstValue();
                return IRValue(Type::Int, std::to_string(result));
            }
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = eq " + eq_exp_value.ToString() + ", " + rel_exp_value.ToString() + "\n");
            return IRValue(Type::Reigster, reg);
        }
        else if (op == "!=")
        {
            IRValue eq_exp_value = eq_exp->GenerateIRExpr(builder);
            IRValue rel_exp_value = rel_exp->GenerateIRExpr(builder);
            if (eq_exp_value.IsConst() && rel_exp_value.IsConst())
            {
                int result = eq_exp_value.GetConstValue() != rel_exp_value.GetConstValue();
                return IRValue(Type::Int, std::to_string(result));
            }
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = ne " + eq_exp_value.ToString() + ", " + rel_exp_value.ToString() + "\n");
            return IRValue(Type::Reigster, reg);
        }
        else
        {
            abort();
        }
    }
};

class LAndExpAST : public BaseExpAST
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

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return eq_exp->GenerateIRExpr(builder);
        }
        else if (op == "&&")
        {
            IRValue land_exp_value = land_exp->GenerateIRExpr(builder);
            IRValue eq_exp_value = eq_exp->GenerateIRExpr(builder);
            if (land_exp_value.IsConst() && eq_exp_value.IsConst())
            {
                int result = land_exp_value.GetConstValue() && eq_exp_value.GetConstValue();
                return IRValue(Type::Int, std::to_string(result));
            }
            std::string reg1 = builder.NewReg();
            builder.Emit("  " + reg1 + " = ne " + land_exp_value.ToString() + ", " + "0" + "\n");
            std::string reg2 = builder.NewReg();
            builder.Emit("  " + reg2 + " = ne " + eq_exp_value.ToString() + ", " + "0" + "\n");
            std::string reg3 = builder.NewReg();
            builder.Emit("  " + reg3 + " = and " + reg1 + ", " + reg2 + "\n");
            return IRValue(Type::Reigster, reg3);
        }
        else
        {
            abort();
        }
    }
};

class LOrExpAST : public BaseExpAST
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

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        if (op == "")
        {
            return land_exp->GenerateIRExpr(builder);
        }
        else if (op == "||")
        {
            IRValue lor_exp_value = lor_exp->GenerateIRExpr(builder);
            IRValue land_exp_value = land_exp->GenerateIRExpr(builder);
            if (lor_exp_value.IsConst() && land_exp_value.IsConst())
            {
                int result = lor_exp_value.GetConstValue() || land_exp_value.GetConstValue();
                return IRValue(Type::Int, std::to_string(result));
            }
            std::string reg1 = builder.NewReg();
            builder.Emit("  " + reg1 + " = ne " + lor_exp_value.ToString() + ", " + "0" + "\n");
            std::string reg2 = builder.NewReg();
            builder.Emit("  " + reg2 + " = ne " + land_exp_value.ToString() + ", " + "0" + "\n");
            std::string reg3 = builder.NewReg();
            builder.Emit("  " + reg3 + " = or " + reg1 + ", " + reg2 + "\n");
            return IRValue(Type::Reigster, reg3);
        }
        else
        {
            abort();
        }
    }
};

class UnaryExpAST : public BaseExpAST
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

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        IRValue ir_value = exp_ast->GenerateIRExpr(builder);
        std::string value = ir_value.ToString();
        if (unaryOp == "+")
        {
            return ir_value;
        }
        else if (unaryOp == "-")
        {
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = sub 0, " + value + "\n");
            return IRValue(Type::Reigster, reg);
        }
        else if (unaryOp == "!")
        {
            std::string reg = builder.NewReg();
            builder.Emit("  " + reg + " = eq " + value + ", 0" + +"\n");
            return IRValue(Type::Reigster, reg);
        }
        else
        {
            if (unaryOp != "")
                abort();
            return ir_value;
        }
    }
};

class PrimaryExpAST : public BaseExpAST
{
public:
    std::unique_ptr<BaseAST> exp_ast;
    void Dump() const override
    {
        std::cout << "PrimaryExp { ";
        exp_ast->Dump();
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return exp_ast->GenerateIRExpr(builder);
    }
};

class ParenExpAST : public BaseExpAST
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

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return exp_ast->GenerateIRExpr(builder);
    }
};

class NumberExpAST : public BaseExpAST
{
public:
    int number;
    void Dump() const override
    {
        std::cout << "NumberExp { ";
        std::cout << number;
        std::cout << " } ";
    }

    IRValue GenerateIRExpr(IRBuilder &builder) const override
    {
        return IRValue(Type::Int, std::to_string(number));
    }
};