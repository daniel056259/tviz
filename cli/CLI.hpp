#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <unordered_map>
#include <memory>
#include <functional>
#include <fstream>
#include <cctype>
#include <stdexcept>
#include "../tree/node.hpp"
#include "../tree/tree.hpp"
#include "../visualizer/visualizer.hpp"

using std::string;
using std::vector;
using std::unique_ptr;
using std::unordered_map;
using std::function;
using std::runtime_error;
using std::ostringstream;
using std::cout;
using std::cin;
using std::cerr;

// ----- recorder state for input logging -----
struct Recorder {
    static std::ofstream out;
    static bool enabled;
};
std::ofstream Recorder::out;
bool Recorder::enabled = false;

// ========================= Value & helpers =========================
using Int = long long;
using Num = double;
using Value = std::variant<Int, Num, string, bool>;

static bool is_numeric_like(const Value& v) {
    return std::holds_alternative<Int>(v) || std::holds_alternative<Num>(v) || std::holds_alternative<bool>(v);
}

static Num as_number(const Value& v) {
    if (std::holds_alternative<Int>(v)) return static_cast<Num>(std::get<Int>(v));
    if (std::holds_alternative<Num>(v)) return std::get<Num>(v);
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? 1.0 : 0.0;
    throw runtime_error("Type error: expected number");
}

static string toString_value(const Value& v) {
    if (std::holds_alternative<Int>(v)) return std::to_string(std::get<Int>(v));
    if (std::holds_alternative<Num>(v)) {
        ostringstream oss; oss << std::setprecision(15) << std::get<Num>(v); return oss.str();
    }
    if (std::holds_alternative<string>(v)) return std::get<string>(v);
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? "true" : "false";
    return "<unknown>";
}

// ========================= Tokens =========================
enum class TokenType {
    IDENTIFIER, INT, FLOAT, STRING,
    PLUS, MINUS, MUL, DIV, MOD,
    ASSIGNMENT,     // =
    EQUAL,          // ==
    NEQUAL,         // !=
    LT, GT, LE, GE, // < > <= >=
    COMMA, SEMICOLON,
    LPAREN, RPAREN,
    LBRACE, RBRACE,
    LBRACKET, RBRACKET,
    KW_IF, KW_ELIF, KW_ELSE,
    KW_FOR, KW_WHILE,
    KW_VOID, KW_INT, KW_FLOAT, KW_STRING, KW_BOOL,
    KW_TRUE, KW_FALSE,
    KW_CONTINUE, KW_BREAK,
    END
};

using TokenValue = std::variant<std::monostate, Int, Num, string, bool>;

struct Token {
    TokenType type{};
    TokenValue value{};
};

// ========================= Lexer =========================
class Lexer {
    string src;
    size_t i = 0, n = 0;
    vector<Token> out;

    char peek() const { return i < n ? src[i] : '\0'; }
    char get() { return i < n ? src[i++] : '\0'; }

    static bool is_ident_start(char c) {
        unsigned char uc = static_cast<unsigned char>(c);
        return std::isalpha(uc) || c == '_';
    }
    static bool is_ident_char(char c) {
        unsigned char uc = static_cast<unsigned char>(c);
        return std::isalnum(uc) || c == '_';
    }
    void skip_ws() {
        while (i < n && std::isspace(static_cast<unsigned char>(src[i]))) ++i;
    }

    Token make(TokenType t) { return Token{ t, {} }; }
    Token make(TokenType t, Int v) { return Token{ t, TokenValue{ v } }; }
    Token make(TokenType t, Num v) { return Token{ t, TokenValue{ v } }; }
    Token make(TokenType t, string v) { return Token{ t, TokenValue{ std::move(v) } }; }
    Token make_bool(bool b) { return Token{ b ? TokenType::KW_TRUE : TokenType::KW_FALSE, TokenValue{ b } }; }

    Token scan_number() {
        bool is_float = false;
        string buf;
        while (std::isdigit(static_cast<unsigned char>(peek()))) buf.push_back(get());
        if (peek() == '.') {
            is_float = true; buf.push_back(get());
            while (std::isdigit(static_cast<unsigned char>(peek()))) buf.push_back(get());
        }
        return is_float ? make(TokenType::FLOAT, std::stod(buf))
            : make(TokenType::INT, (Int)std::stoll(buf));
    }

    Token scan_identifier_or_keyword() {
        string buf; buf.push_back(get());
        while (is_ident_char(peek())) buf.push_back(get());

        if (buf == "if")       return make(TokenType::KW_IF);
        if (buf == "elif")     return make(TokenType::KW_ELIF);
        if (buf == "else")     return make(TokenType::KW_ELSE);
        if (buf == "for")      return make(TokenType::KW_FOR);
        if (buf == "while")    return make(TokenType::KW_WHILE);
        if (buf == "void")     return make(TokenType::KW_VOID);
        if (buf == "int")      return make(TokenType::KW_INT);
        if (buf == "float")    return make(TokenType::KW_FLOAT);
        if (buf == "string")   return make(TokenType::KW_STRING);
        if (buf == "bool")     return make(TokenType::KW_BOOL);
        if (buf == "true")     return make_bool(true);
        if (buf == "false")    return make_bool(false);
        if (buf == "continue") return make(TokenType::KW_CONTINUE);
        if (buf == "break")    return make(TokenType::KW_BREAK);

        return make(TokenType::IDENTIFIER, buf);
    }

    Token scan_string() {
        if (get() != '"') throw runtime_error("String must start with '\"'");
        string buf;
        while (true) {
            char c = get();
            if (c == '\0') throw runtime_error("Unterminated string literal");
            if (c == '"') break;
            if (c == '\\') {
                char e = get();
                switch (e) {
                case 'n': buf.push_back('\n'); break;
                case 't': buf.push_back('\t'); break;
                case '"': buf.push_back('"'); break;
                case '\\': buf.push_back('\\'); break;
                default: buf.push_back(e); break;
                }
            }
            else buf.push_back(c);
        }
        return make(TokenType::STRING, buf);
    }

public:
    explicit Lexer(string s) : src(std::move(s)) { n = src.size(); }

    vector<Token> tokenize() {
        while (true) {
            skip_ws();
            char c = peek();
            if (c == '\0') { out.push_back(make(TokenType::END)); break; }
            if (std::isdigit(static_cast<unsigned char>(c))) { out.push_back(scan_number()); continue; }
            if (is_ident_start(c)) { out.push_back(scan_identifier_or_keyword()); continue; }
            if (c == '"') { out.push_back(scan_string()); continue; }

            switch (c) {
            case '+': out.push_back(make(TokenType::PLUS));      ++i; break;
            case '-': out.push_back(make(TokenType::MINUS));     ++i; break;
            case '*': out.push_back(make(TokenType::MUL));       ++i; break;
            case '/': out.push_back(make(TokenType::DIV));       ++i; break;
            case '%': out.push_back(make(TokenType::MOD));       ++i; break;
            case ',': out.push_back(make(TokenType::COMMA));     ++i; break;
            case ';': out.push_back(make(TokenType::SEMICOLON)); ++i; break;
            case '(': out.push_back(make(TokenType::LPAREN));    ++i; break;
            case ')': out.push_back(make(TokenType::RPAREN));    ++i; break;
            case '{': out.push_back(make(TokenType::LBRACE));    ++i; break;
            case '}': out.push_back(make(TokenType::RBRACE));    ++i; break;
            case '[': out.push_back(make(TokenType::LBRACKET));  ++i; break;
            case ']': out.push_back(make(TokenType::RBRACKET));  ++i; break;

            case '!':
                if (i + 1 < n && src[i + 1] == '=') { out.push_back(make(TokenType::NEQUAL)); i += 2; }
                else { throw runtime_error("Unexpected '!' (only '!=' supported)"); }
                break;
            case '<':
                if (i + 1 < n && src[i + 1] == '=') { out.push_back(make(TokenType::LE)); i += 2; }
                else { out.push_back(make(TokenType::LT)); ++i; }
                break;
            case '>':
                if (i + 1 < n && src[i + 1] == '=') { out.push_back(make(TokenType::GE)); i += 2; }
                else { out.push_back(make(TokenType::GT)); ++i; }
                break;
            case '=':
                if (i + 1 < n && src[i + 1] == '=') { out.push_back(make(TokenType::EQUAL)); i += 2; }
                else { out.push_back(make(TokenType::ASSIGNMENT)); ++i; }
                break;

            default: throw runtime_error("Unexpected character");
            }
        }
        return out;
    }
};

// ========================= Environment =========================
enum class VariableType { INT, FLOAT, STRING, BOOL };

struct VariableInfo {
    std::optional<VariableType> declared_type;
    bool initialized = false;
    Value val{};
};

class Environment {
public:
    using Builtin = function<Value(const vector<Value>&, Environment&)>;
private:
    vector<unordered_map<string, VariableInfo>> scopes;
    unordered_map<string, Builtin> builtins;
public:
    Environment() { scopes.push_back({}); }

    void push_scope() { scopes.push_back({}); }
    void pop_scope() {
        if (scopes.size() <= 1) throw runtime_error("Cannot pop global scope");
        scopes.pop_back();
    }

    void declare_uninitialized(const string& id, std::optional<VariableType> ty = std::nullopt) {
        auto& cur = scopes.back();
        if (cur.count(id)) throw runtime_error("Redeclare in same scope: " + id);
        cur[id] = VariableInfo{ ty, false, Value{} };
    }

    static VariableType infer_type(const Value& v) {
        if (std::holds_alternative<Int>(v))    return VariableType::INT;
        if (std::holds_alternative<Num>(v))    return VariableType::FLOAT;
        if (std::holds_alternative<string>(v)) return VariableType::STRING;
        if (std::holds_alternative<bool>(v))   return VariableType::BOOL;
        throw runtime_error("Unknown value type");
    }

    void assign(const string& id, Value v) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto f = it->find(id);
            if (f != it->end()) {
                if (f->second.declared_type.has_value()) {
                    if (*f->second.declared_type != infer_type(v))
                        throw runtime_error("Type mismatch on assign to " + id);
                }
                else {
                    f->second.declared_type = infer_type(v);
                }
                f->second.val = std::move(v);
                f->second.initialized = true;
                return;
            }
        }
        throw runtime_error("Undeclared variable: " + id);
    }

    Value get_var_checked(const string& id) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto f = it->find(id);
            if (f != it->end()) {
                if (!f->second.initialized) throw runtime_error("Use of uninitialized variable: " + id);
                return f->second.val;
            }
        }
        throw runtime_error("Undeclared variable: " + id);
    }

    void set_builtin(const string& name, Builtin fn) { builtins[name] = std::move(fn); }
    Value call_builtin(const string& name, const vector<Value>& args) {
        auto it = builtins.find(name);
        if (it == builtins.end()) throw runtime_error("Unknown function: " + name);
        return it->second(args, *this);
    }

    // -------- File I/O helpers for globals --------
    void export_globals_to_file(const string& path) const {
        std::ofstream out(path, std::ios::binary);
        if (!out) throw runtime_error("Failed to open file for export: " + path);
        const auto& g = scopes.front();
        for (const auto& kv : g) {
            const string& name = kv.first;
            const VariableInfo& vi = kv.second;
            if (!vi.initialized || !vi.declared_type.has_value()) continue;
            string type;
            string val;
            switch (*vi.declared_type) {
            case VariableType::INT:    type = "INT";    val = std::to_string(std::get<Int>(vi.val)); break;
            case VariableType::FLOAT:  type = "FLOAT"; { ostringstream oss; oss << std::setprecision(15) << std::get<Num>(vi.val); val = oss.str(); } break;
            case VariableType::STRING: type = "STRING"; {
                val = std::get<string>(vi.val);
                string esc; esc.reserve(val.size());
                for (char c : val) { if (c == '\\' || c == '"') { esc.push_back('\\'); esc.push_back(c); } else if (c == '\n') { esc += "\\n"; } else if (c == '\t') { esc += "\\t"; } else esc.push_back(c); }
                val = esc;
            } break;
            case VariableType::BOOL:   type = "BOOL";   val = std::get<bool>(vi.val) ? "true" : "false"; break;
            }
            out << name << '\t' << type << '\t' << val << "\n";
        }
    }

    void import_globals_from_file(const string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) throw runtime_error("Failed to open file for import: " + path);
        auto& g = scopes.front();
        string name, type, val;
        string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            if (!std::getline(iss, name, '\t')) continue;
            if (!std::getline(iss, type, '\t')) continue;
            if (!std::getline(iss, val)) val.clear();
            auto unescape = [](const string& s)->string {
                string r; r.reserve(s.size());
                for (size_t k = 0; k < s.size(); ++k) {
                    if (s[k] == '\\' && k + 1 < s.size()) {
                        char e = s[++k];
                        if (e == 'n') r.push_back('\n');
                        else if (e == 't') r.push_back('\t');
                        else r.push_back(e);
                    }
                    else r.push_back(s[k]);
                }
                return r;
            };
            if (type == "INT") {
                Value v = (Int)std::stoll(val);
                g[name] = VariableInfo{ VariableType::INT, true, std::move(v) };
            }
            else if (type == "FLOAT") {
                Value v = (Num)std::stod(val);
                g[name] = VariableInfo{ VariableType::FLOAT, true, std::move(v) };
            }
            else if (type == "STRING") {
                Value v = unescape(val);
                g[name] = VariableInfo{ VariableType::STRING, true, std::move(v) };
            }
            else if (type == "BOOL") {
                Value v = (val == "true");
                g[name] = VariableInfo{ VariableType::BOOL, true, std::move(v) };
            }
        }
    }
};

// ========================= AST =========================
struct AstNode : public Node {
    virtual ~AstNode() = default;
    virtual Value eval(Environment& env) = 0;
    int getKeyCount() const override { return 1; }
};

struct LiteralNode : AstNode {
    Value v; 
    explicit LiteralNode(Value vv) : v(std::move(vv)) {}
    Value eval(Environment&) override { return v; }
    void draw(Visualizer& vis) override;
};

struct VarRefNode : AstNode {
    string name; 
    explicit VarRefNode(string n) : name(std::move(n)) {}
    Value eval(Environment& env) override { return env.get_var_checked(name); }
    void draw(Visualizer& vis) override;
};

struct CallNode : AstNode {
    string callee; vector<unique_ptr<AstNode>> args;
    CallNode(string c, vector<unique_ptr<AstNode>> a) : callee(std::move(c)), args(std::move(a)) {}
    Value eval(Environment& env) override {
        vector<Value> vs; vs.reserve(args.size());
        for (auto& p : args) vs.push_back(p->eval(env));
        return env.call_builtin(callee, vs);
    }
    void draw(Visualizer& vis) override;
};

struct AssignNode : AstNode {
    string name; unique_ptr<AstNode> expr;
    AssignNode(string n, unique_ptr<AstNode> e) : name(std::move(n)), expr(std::move(e)) {}
    Value eval(Environment& env) override {
        Value v = expr->eval(env);
        env.assign(name, v);
        return v;
    }
    void draw(Visualizer& vis) override;
};

struct TypedDeclNode : AstNode {
    string name; VariableType vt;
    TypedDeclNode(string n, VariableType v) : name(std::move(n)), vt(v) {}
    Value eval(Environment& env) override {
        if (vt == VariableType::INT || vt == VariableType::FLOAT ||
            vt == VariableType::STRING || vt == VariableType::BOOL) {
            env.declare_uninitialized(name, vt);
            return Value{ Int(0) };
        }
        throw runtime_error("void variable is not allowed");
    }
    void draw(Visualizer& vis) override;
};

struct TypedInitNode : AstNode {
    string name; VariableType vt; unique_ptr<AstNode> expr;
    TypedInitNode(string n, VariableType v, unique_ptr<AstNode> e)
        : name(std::move(n)), vt(v), expr(std::move(e)) {
    }
    Value eval(Environment& env) override {
        Value val = expr->eval(env);
        if (Environment::infer_type(val) != vt) throw runtime_error("Type mismatch on initialization: " + name);
        env.declare_uninitialized(name, vt);
        env.assign(name, val);
        return val;
    }
    void draw(Visualizer& vis) override;
};

enum class BinOp { Add, Sub, Mul, Div, Mod, Eq, Ne, Lt, Gt, Le, Ge };
enum class UnOp { Plus, Neg };

static string binOpToString(BinOp op) {
    switch(op) {
        case BinOp::Add: return "+"; case BinOp::Sub: return "-";
        case BinOp::Mul: return "*"; case BinOp::Div: return "/"; case BinOp::Mod: return "%";
        case BinOp::Eq: return "=="; case BinOp::Ne: return "!=";
        case BinOp::Lt: return "<";  case BinOp::Gt: return ">";
        case BinOp::Le: return "<="; case BinOp::Ge: return ">=";
    }
    return "?";
}

struct BinaryOpNode : AstNode {
    BinOp op;
    unique_ptr<AstNode> lhs, rhs;
    BinaryOpNode(BinOp o, unique_ptr<AstNode> L, unique_ptr<AstNode> R)
        : op(o), lhs(std::move(L)), rhs(std::move(R)) {
    }

    Value eval(Environment& env) override {
        // comparisons
        if (op == BinOp::Eq || op == BinOp::Ne ||
            op == BinOp::Lt || op == BinOp::Gt ||
            op == BinOp::Le || op == BinOp::Ge) {
            Value lv = lhs->eval(env), rv = rhs->eval(env);

            if (std::holds_alternative<string>(lv) || std::holds_alternative<string>(rv)) {
                if (!(std::holds_alternative<string>(lv) && std::holds_alternative<string>(rv)))
                    return Value{ false };
                const auto& a = std::get<string>(lv);
                const auto& b = std::get<string>(rv);
                bool res = false;
                switch (op) {
                case BinOp::Eq: res = (a == b); break;
                case BinOp::Ne: res = (a != b); break;
                case BinOp::Lt: res = (a < b); break;
                case BinOp::Gt: res = (a > b); break;
                case BinOp::Le: res = (a <= b); break;
                case BinOp::Ge: res = (a >= b); break;
                default: break;
                }
                return Value{ res };
            }

            if (is_numeric_like(lv) && is_numeric_like(rv)) {
                const double a = as_number(lv);
                const double b = as_number(rv);
                bool res = false;
                switch (op) {
                case BinOp::Eq: res = (a == b); break;
                case BinOp::Ne: res = (a != b); break;
                case BinOp::Lt: res = (a < b); break;
                case BinOp::Gt: res = (a > b); break;
                case BinOp::Le: res = (a <= b); break;
                case BinOp::Ge: res = (a >= b); break;
                default: break;
                }
                return Value{ res };
            }

            return Value{ false };
        }

        // arithmetic
        Value la = lhs->eval(env), rb = rhs->eval(env);
        bool la_int = std::holds_alternative<Int>(la) || std::holds_alternative<bool>(la);
        bool rb_int = std::holds_alternative<Int>(rb) || std::holds_alternative<bool>(rb);

        auto get_int = [](const Value& v)->Int {
            if (std::holds_alternative<Int>(v))  return std::get<Int>(v);
            if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? 1 : 0;
            throw runtime_error("Expected integer-like value");
        };

        switch (op) {
        case BinOp::Add:
            if (la_int && rb_int) return Value{ get_int(la) + get_int(rb) };
            return Value{ as_number(la) + as_number(rb) };
        case BinOp::Sub:
            if (la_int && rb_int) return Value{ get_int(la) - get_int(rb) };
            return Value{ as_number(la) - as_number(rb) };
        case BinOp::Mul:
            if (la_int && rb_int) return Value{ get_int(la) * get_int(rb) };
            return Value{ as_number(la) * as_number(rb) };
        case BinOp::Div: {
            const double denom = as_number(rb);
            if (denom == 0.0) throw runtime_error("Division by zero");
            return Value{ as_number(la) / denom };
        }
        case BinOp::Mod: {
            Int a = get_int(la), b = get_int(rb);
            if (b == 0) throw runtime_error("Modulo by zero");
            return Value{ static_cast<Int>(a % b) };
        }
        default: throw runtime_error("unsupported binary op");
        }
    }
    void draw(Visualizer& vis) override;
};

struct UnaryOpNode : AstNode {
    UnOp op; unique_ptr<AstNode> inner;
    UnaryOpNode(UnOp o, unique_ptr<AstNode> in) : op(o), inner(std::move(in)) {}
    Value eval(Environment& env) override {
        Value v = inner->eval(env);
        if (std::holds_alternative<Int>(v)) {
            Int i = std::get<Int>(v);
            return (op == UnOp::Plus) ? Value{ i } : Value{ -i };
        }
        double x = as_number(v);
        return (op == UnOp::Plus) ? Value{ x } : Value{ -x };
    }
    void draw(Visualizer& vis) override;
};

struct BreakSignal {};
struct ContinueSignal {};

struct BreakNode : AstNode { 
    Value eval(Environment&) override { throw BreakSignal{}; } 
    void draw(Visualizer& vis) override;
};
struct ContinueNode : AstNode { 
    Value eval(Environment&) override { throw ContinueSignal{}; } 
    void draw(Visualizer& vis) override;
};

struct BlockNode : AstNode {
    vector<unique_ptr<AstNode>> stmts;
    Value eval(Environment& env) override {
        env.push_scope();
        try {
            Value last = Int(0);
            for (auto& s : stmts) last = s->eval(env);
            env.pop_scope();
            return last;
        }
        catch (...) {
            try { env.pop_scope(); }
            catch (...) {}
            throw;
        }
    }
    void draw(Visualizer& vis) override;
};

struct IfNode : AstNode {
    struct Arm { unique_ptr<AstNode> cond; unique_ptr<AstNode> thenStmt; };
    vector<Arm> arms; unique_ptr<AstNode> elseStmt;
    Value eval(Environment& env) override {
        for (auto& a : arms) {
            Value cv = a.cond->eval(env);
            bool truthy = std::holds_alternative<bool>(cv) ? std::get<bool>(cv)
                : (as_number(cv) != 0.0);
            if (truthy) return a.thenStmt->eval(env);
        }
        if (elseStmt) return elseStmt->eval(env);
        return Value{ Int(0) };
    }
    void draw(Visualizer& vis) override;
};

struct WhileNode : AstNode {
    unique_ptr<AstNode> cond, body;
    WhileNode(unique_ptr<AstNode> c, unique_ptr<AstNode> b) : cond(std::move(c)), body(std::move(b)) {}
    Value eval(Environment& env) override {
        Value last = Int(0);
        while (true) {
            Value cv = cond->eval(env);
            bool truthy = std::holds_alternative<bool>(cv) ? std::get<bool>(cv)
                : (as_number(cv) != 0.0);
            if (!truthy) break;
            try { last = body->eval(env); }
            catch (const ContinueSignal&) {}
            catch (const BreakSignal&) { break; }
        }
        return last;
    }
    void draw(Visualizer& vis) override;
};

struct ForNode : AstNode {
    unique_ptr<AstNode> init, cond, post, body;
    ForNode(unique_ptr<AstNode> i, unique_ptr<AstNode> c,
        unique_ptr<AstNode> p, unique_ptr<AstNode> b)
        : init(std::move(i)), cond(std::move(c)), post(std::move(p)), body(std::move(b)) {
    }
    Value eval(Environment& env) override {
        env.push_scope();
        try {
            Value last = Int(0);
            if (init) init->eval(env);
            while (true) {
                bool ok = true;
                if (cond) {
                    Value cv = cond->eval(env);
                    ok = std::holds_alternative<bool>(cv) ? std::get<bool>(cv)
                        : (as_number(cv) != 0.0);
                }
                if (!ok) break;
                try { last = body->eval(env); }
                catch (const ContinueSignal&) { /* proceed to post */ }
                catch (const BreakSignal&) { break; }
                if (post) post->eval(env);
            }
            env.pop_scope();
            return last;
        }
        catch (...) {
            try { env.pop_scope(); }
            catch (...) {}
            throw;
        }
    }
    void draw(Visualizer& vis) override;
};

struct ProgramNode : AstNode {
    vector<unique_ptr<AstNode>> stmts;

    ProgramNode() = default;
    ProgramNode(vector<unique_ptr<AstNode>> s) : stmts(std::move(s)) {}

    Value eval(Environment& env) {
        Value last = Int(0);
        for (auto& s : stmts) last = s->eval(env);
        return last;
    }

    void draw(Visualizer& vis) override {
        vis.printLabel(Pos::MID_NORM, "PROGRAM");
        for (int i = 0; i < stmts.size() - 1; i++) {
            vis.printChild(stmts[i].get(), Pos::MID_NORM, this, Pos::DOWN);
        }
        if (stmts.size() > 0)
            vis.printChild(stmts[stmts.size() - 1].get(), Pos::DOWN, this, Pos::DOWN);
    }
};

struct Program {
    vector<unique_ptr<AstNode>> stmts;
    Value execute(Environment& env) {
        Value last = Int(0);
        for (auto& s : stmts) last = s->eval(env);
        return last;
    }
};

// ================ Visualizer Draw Implements ==============
void LiteralNode::draw(Visualizer& vis) {
    vis.printLabel(Pos::MID_NORM, toString_value(v));
}

void VarRefNode::draw(Visualizer& vis) {
    vis.printLabel(Pos::MID_NORM, "VAR: " + name);
}

void CallNode::draw(Visualizer& vis) {
    vis.printLabel(Pos::MID_NORM, "CALL: " + callee);
    for (int i = 0; i < args.size() - 1; i++) {
        vis.printChild(args[i].get(), Pos::MID_NORM, this, Pos::DOWN);
    }
    if (args.size() >= 1)
        vis.printChild(args[args.size() - 1].get(), Pos::DOWN, this, Pos::DOWN);
}

void AssignNode::draw(Visualizer& vis) {
    vis.printLabel(Pos::MID_NORM, "ASSIGN: " + name);
    vis.printChild(expr.get(), Pos::DOWN, this, Pos::DOWN);
}

static string variableTypeToString(VariableType vt) {
    switch (vt)
    {
    case VariableType::INT: return "INT";
    case VariableType::FLOAT: return "FLOAT";
    case VariableType::STRING: return "STRING";
    case VariableType::BOOL: return "BOOL";
    }
    return "ERROR: Unexpected VariableType";
}

void TypedDeclNode::draw(Visualizer& vis) {
    vis.printLabel(Pos::MID_NORM, "DECL: {TYPE: " + variableTypeToString(vt) + ", NAME: " + name + "}");
}

void TypedInitNode::draw(Visualizer& vis) {
    vis.printLabel(Pos::MID_NORM, "INIT: " + name);
    vis.printChild(expr.get(), Pos::DOWN, this, Pos::DOWN);
}

void BinaryOpNode::draw(Visualizer& vis) {
    if (lhs) vis.printChild(lhs.get(), Pos::UP, this, Pos::UP);
    vis.printLabel(Pos::MID_NORM, "OP: " + binOpToString(op));
    if (rhs) vis.printChild(rhs.get(), Pos::DOWN, this, Pos::DOWN);
}

void UnaryOpNode::draw(Visualizer& vis) {
    string op_str = (op == UnOp::Plus) ? "+" : "-";
    vis.printLabel(Pos::MID_NORM, "UNARY: " + op_str);
    if (inner) vis.printChild(inner.get(), Pos::DOWN, this, Pos::DOWN);
}

void BreakNode::draw(Visualizer& vis) {
    vis.printLabel(Pos::MID_NORM, "BREAK");
}

void ContinueNode::draw(Visualizer& vis) {
    vis.printLabel(Pos::MID_NORM, "CONTINUE");
}

void BlockNode::draw(Visualizer& vis) {
    vis.printLabel(Pos::MID_NORM, "BLOCK");
    for (int i = 0; i < stmts.size() - 1; i++) {
        vis.printChild(stmts[i].get(), Pos::MID_NORM, this, Pos::DOWN);
    }
    if (stmts.size() >= 1) vis.printChild(stmts[stmts.size() - 1].get(), Pos::DOWN, this, Pos::DOWN);
}

void IfNode::draw(Visualizer& vis) {
    vis.printLabel(Pos::MID_NORM, "IF");
    if (arms.size() > 0) vis.printKeyConnection(this, Pos::DOWN);
    for (int i = 0; i < arms.size() - 1; i++) {
        vis.printLabel(Pos::DOWN, "[Cond]");
        vis.printChild(arms[i].cond.get(), Pos::MID_NORM, this, Pos::DOWN);
        vis.printLabel(Pos::DOWN, "[Then]");
        vis.printChild(arms[i].thenStmt.get(), Pos::MID_NORM, this, Pos::DOWN);
    }
    if (!elseStmt) {
        vis.printLabel(Pos::DOWN, "[Cond]");
        vis.printChild(arms[arms.size() - 1].cond.get(), Pos::MID_NORM, this, Pos::DOWN);
        vis.printLabel(Pos::DOWN, "[Then]");
        vis.printChild(arms[arms.size() - 1].thenStmt.get(), Pos::DOWN, this, Pos::DOWN);
    }
    else {
        vis.printLabel(Pos::DOWN, "[Cond]");
        vis.printChild(arms[arms.size() - 1].cond.get(), Pos::MID_NORM, this, Pos::DOWN);
        vis.printLabel(Pos::DOWN, "[Then]");
        vis.printChild(arms[arms.size() - 1].thenStmt.get(), Pos::MID_NORM, this, Pos::DOWN);

        vis.printLabel(Pos::DOWN, "[Else]");
        vis.printChild(elseStmt.get(), Pos::DOWN, this, Pos::DOWN);
    }
}

void WhileNode::draw(Visualizer& vis) {
    vis.printLabel(Pos::MID_NORM, "WHILE");
    vis.printKeyConnection(this, Pos::MID_NORM);
    vis.printLabel(Pos::MID_NORM, "[Cond]");
    vis.printChild(cond.get(), Pos::MID_NORM, this, Pos::DOWN);
    vis.printLabel(Pos::MID_NORM, "[Body]");
    vis.printChild(body.get(), Pos::DOWN, this, Pos::DOWN);
}

void ForNode::draw(Visualizer& vis) {
    vis.printLabel(Pos::MID_NORM, "FOR");
    vis.printKeyConnection(this, Pos::DOWN);
    if (init) {
        vis.printLabel(Pos::DOWN, "[Init]");
        vis.printChild(init.get(), Pos::MID_NORM, this, Pos::DOWN);
    }
    if (cond) {
        vis.printLabel(Pos::DOWN, "[Cond]");
        vis.printChild(cond.get(), Pos::MID_NORM, this, Pos::DOWN);
    }
    if (post) {
        vis.printLabel(Pos::DOWN, "[Post]");
        vis.printChild(post.get(), Pos::MID_NORM, this, Pos::DOWN);
    }
    vis.printLabel(Pos::DOWN, "[Body]");
    vis.printChild(body.get(), Pos::DOWN, this, Pos::DOWN);
}

// ========================= Parser =========================
class Parser {
    const vector<Token> toks;
    size_t i = 0;

    const Token& cur() const { return toks[i]; }
    bool check(TokenType t) const { return cur().type == t; }
    bool match(TokenType t) { if (check(t)) { ++i; return true; } return false; }
    const Token& consume(TokenType t, const char* msg) {
        if (!check(t)) throw runtime_error(string("Parse error: expected ") + msg);
        return toks[i++];
    }
    bool at_end() const { return check(TokenType::END); }

    static bool is_type_token(TokenType t) {
        return t == TokenType::KW_INT || t == TokenType::KW_FLOAT ||
            t == TokenType::KW_STRING || t == TokenType::KW_BOOL ||
            t == TokenType::KW_VOID;
    }

    unique_ptr<AstNode> parse_statement() {
        if (match(TokenType::LBRACE)) return parse_block_after_lbrace();

        if (is_type_token(cur().type)) return parse_typed_decl_stmt();

        if (match(TokenType::KW_IF))    return parse_if_stmt();
        if (match(TokenType::KW_WHILE)) return parse_while_stmt();
        if (match(TokenType::KW_FOR))   return parse_for_stmt();

        if (match(TokenType::KW_BREAK)) { consume(TokenType::SEMICOLON, "';'"); return std::make_unique<BreakNode>(); }
        if (match(TokenType::KW_CONTINUE)) { consume(TokenType::SEMICOLON, "';'"); return std::make_unique<ContinueNode>(); }

        if (check(TokenType::IDENTIFIER) && toks[i + 1].type == TokenType::ASSIGNMENT) {
            auto id = consume(TokenType::IDENTIFIER, "identifier");
            const auto& name = std::get<string>(id.value);
            consume(TokenType::ASSIGNMENT, "'='");
            auto e = parse_expr();
            consume(TokenType::SEMICOLON, "';'");
            return std::make_unique<AssignNode>(name, std::move(e));
        }

        auto e = parse_expr();
        consume(TokenType::SEMICOLON, "';'");
        return e;
    }

    unique_ptr<AstNode> parse_block_after_lbrace() {
        auto blk = std::make_unique<BlockNode>();
        while (!check(TokenType::RBRACE)) blk->stmts.push_back(parse_statement());
        consume(TokenType::RBRACE, "'}'");
        return blk;
    }

    unique_ptr<AstNode> parse_typed_decl_stmt() {
        VariableType vt = parse_type_spec();
        auto id = consume(TokenType::IDENTIFIER, "identifier");
        const auto& name = std::get<string>(id.value);
        if (match(TokenType::SEMICOLON)) {
            return std::make_unique<TypedDeclNode>(name, vt);
        }
        consume(TokenType::ASSIGNMENT, "'='");
        auto e = parse_expr();
        consume(TokenType::SEMICOLON, "';'");
        return std::make_unique<TypedInitNode>(name, vt, std::move(e));
    }

    VariableType parse_type_spec() {
        if (match(TokenType::KW_INT))    return VariableType::INT;
        if (match(TokenType::KW_FLOAT))  return VariableType::FLOAT;
        if (match(TokenType::KW_STRING)) return VariableType::STRING;
        if (match(TokenType::KW_BOOL))   return VariableType::BOOL;
        if (match(TokenType::KW_VOID))   throw runtime_error("void variable is not allowed");
        throw runtime_error("Parse error: expected type specifier");
    }

    unique_ptr<AstNode> parse_if_stmt() {
        consume(TokenType::LPAREN, "'('");
        auto cond = parse_expr();
        consume(TokenType::RPAREN, "')'");
        auto thenStmt = parse_branch_body();

        auto node = std::make_unique<IfNode>();
        node->arms.push_back(IfNode::Arm{ std::move(cond), std::move(thenStmt) });

        while (match(TokenType::KW_ELIF)) {
            consume(TokenType::LPAREN, "'('");
            auto c = parse_expr();
            consume(TokenType::RPAREN, "')'");
            auto t = parse_branch_body();
            node->arms.push_back(IfNode::Arm{ std::move(c), std::move(t) });
        }
        if (match(TokenType::KW_ELSE)) node->elseStmt = parse_branch_body();
        return node;
    }

    unique_ptr<AstNode> parse_while_stmt() {
        consume(TokenType::LPAREN, "'('");
        auto cond = parse_expr();
        consume(TokenType::RPAREN, "')'");
        auto body = parse_branch_body();
        return std::make_unique<WhileNode>(std::move(cond), std::move(body));
    }

    unique_ptr<AstNode> parse_for_stmt() {
        consume(TokenType::LPAREN, "'('");
        unique_ptr<AstNode> init, cond, post;
        if (!check(TokenType::SEMICOLON)) init = parse_simple_no_semicolon();
        consume(TokenType::SEMICOLON, "';'");
        if (!check(TokenType::SEMICOLON)) cond = parse_expr();
        consume(TokenType::SEMICOLON, "';'");
        if (!check(TokenType::RPAREN))    post = parse_simple_no_semicolon();
        consume(TokenType::RPAREN, "')'");
        auto body = parse_branch_body();
        return std::make_unique<ForNode>(std::move(init), std::move(cond), std::move(post), std::move(body));
    }

    unique_ptr<AstNode> parse_branch_body() {
        if (match(TokenType::LBRACE)) return parse_block_after_lbrace();
        return parse_statement();
    }

    unique_ptr<AstNode> parse_simple_no_semicolon() {
        if (is_type_token(cur().type)) {
            VariableType vt = parse_type_spec();
            auto id = consume(TokenType::IDENTIFIER, "identifier");
            const auto& name = std::get<string>(id.value);
            if (match(TokenType::ASSIGNMENT)) {
                auto e = parse_expr();
                return std::make_unique<TypedInitNode>(name, vt, std::move(e));
            }
            return std::make_unique<TypedDeclNode>(name, vt);
        }
        if (check(TokenType::IDENTIFIER) && toks[i + 1].type == TokenType::ASSIGNMENT) {
            auto id = consume(TokenType::IDENTIFIER, "identifier");
            const auto& name = std::get<string>(id.value);
            consume(TokenType::ASSIGNMENT, "'='");
            auto e = parse_expr();
            return std::make_unique<AssignNode>(name, std::move(e));
        }
        return parse_expr();
    }

    // expressions with precedence
    unique_ptr<AstNode> parse_expr() { return parse_equality(); }

    unique_ptr<AstNode> parse_equality() {
        auto node = parse_comparison();
        while (true) {
            if (match(TokenType::EQUAL)) { auto r = parse_comparison(); node = std::make_unique<BinaryOpNode>(BinOp::Eq, std::move(node), std::move(r)); continue; }
            if (match(TokenType::NEQUAL)) { auto r = parse_comparison(); node = std::make_unique<BinaryOpNode>(BinOp::Ne, std::move(node), std::move(r)); continue; }
            break;
        }
        return node;
    }

    unique_ptr<AstNode> parse_comparison() {
        auto node = parse_add();
        while (true) {
            if (match(TokenType::LT)) { auto r = parse_add(); node = std::make_unique<BinaryOpNode>(BinOp::Lt, std::move(node), std::move(r)); continue; }
            if (match(TokenType::GT)) { auto r = parse_add(); node = std::make_unique<BinaryOpNode>(BinOp::Gt, std::move(node), std::move(r)); continue; }
            if (match(TokenType::LE)) { auto r = parse_add(); node = std::make_unique<BinaryOpNode>(BinOp::Le, std::move(node), std::move(r)); continue; }
            if (match(TokenType::GE)) { auto r = parse_add(); node = std::make_unique<BinaryOpNode>(BinOp::Ge, std::move(node), std::move(r)); continue; }
            break;
        }
        return node;
    }

    unique_ptr<AstNode> parse_add() {
        auto node = parse_mul();
        while (true) {
            if (match(TokenType::PLUS)) { auto r = parse_mul(); node = std::make_unique<BinaryOpNode>(BinOp::Add, std::move(node), std::move(r)); continue; }
            if (match(TokenType::MINUS)) { auto r = parse_mul(); node = std::make_unique<BinaryOpNode>(BinOp::Sub, std::move(node), std::move(r)); continue; }
            break;
        }
        return node;
    }

    unique_ptr<AstNode> parse_mul() {
        auto node = parse_unary();
        while (true) {
            if (match(TokenType::MUL)) { auto r = parse_unary(); node = std::make_unique<BinaryOpNode>(BinOp::Mul, std::move(node), std::move(r)); continue; }
            if (match(TokenType::DIV)) { auto r = parse_unary(); node = std::make_unique<BinaryOpNode>(BinOp::Div, std::move(node), std::move(r)); continue; }
            if (match(TokenType::MOD)) { auto r = parse_unary(); node = std::make_unique<BinaryOpNode>(BinOp::Mod, std::move(node), std::move(r)); continue; }
            break;
        }
        return node;
    }

    unique_ptr<AstNode> parse_unary() {
        if (match(TokenType::PLUS))  return std::make_unique<UnaryOpNode>(UnOp::Plus, parse_unary());
        if (match(TokenType::MINUS)) return std::make_unique<UnaryOpNode>(UnOp::Neg, parse_unary());
        return parse_primary();
    }

    unique_ptr<AstNode> parse_primary() {
        if (match(TokenType::INT)) { auto v = std::get<Int>(toks[i - 1].value);   return std::make_unique<LiteralNode>(Value{ v }); }
        if (match(TokenType::FLOAT)) { auto v = std::get<Num>(toks[i - 1].value);   return std::make_unique<LiteralNode>(Value{ v }); }
        if (match(TokenType::STRING)) { auto& s = std::get<string>(toks[i - 1].value); return std::make_unique<LiteralNode>(Value{ s }); }
        if (match(TokenType::KW_TRUE))  return std::make_unique<LiteralNode>(Value{ true });
        if (match(TokenType::KW_FALSE)) return std::make_unique<LiteralNode>(Value{ false });

        if (match(TokenType::LPAREN)) {
            auto e = parse_expr(); consume(TokenType::RPAREN, "')'"); return e;
        }

        if (check(TokenType::IDENTIFIER)) {
            auto id = consume(TokenType::IDENTIFIER, "identifier");
            const auto& name = std::get<string>(id.value);
            if (match(TokenType::LPAREN)) {
                vector<unique_ptr<AstNode>> args;
                if (!check(TokenType::RPAREN)) {
                    while (true) { args.push_back(parse_expr()); if (match(TokenType::COMMA)) continue; break; }
                }
                consume(TokenType::RPAREN, "')'");
                return std::make_unique<CallNode>(name, std::move(args));
            }
            return std::make_unique<VarRefNode>(name);
        }

        throw runtime_error("Parse error: unexpected token");
    }

public:
    explicit Parser(vector<Token> t) : toks(std::move(t)) {}

    unique_ptr<ProgramNode> parse() {
        auto prog = std::make_unique<ProgramNode>();
        while (!at_end()) prog->stmts.push_back(parse_statement());
        return prog;
    }
};

class AST : public Tree {
public:
    AST() {
        this->vis = new Visualizer(this);
    }

    ~AST() override { delete vis; }

    Value run(const string& source, Environment& env) {
        try {
            Lexer lx(source);
            auto toks = lx.tokenize();

            Parser ps(std::move(toks));
            this->prog_root = ps.parse();
            this->root_ptr = prog_root.get();

            vis->clear();
            vis->setTitle("AST Visualization");
            vis->setMessage("Executing Program ...");
            VisualizerConfig::setDelayDuration(chrono::milliseconds(0));
            vis->render();

            if (prog_root)
                return prog_root->eval(env);
        }
        catch (const std::exception& e) {
            cout << "\n[Runtime Error] " << e.what() << endl;
        }
        return Value{Int(0)};
    }

private:
    unique_ptr<ProgramNode> prog_root;
};



// ========================= Builtins =========================
static void register_builtins(Environment& env) {
    env.set_builtin("add", [](const vector<Value>& a, Environment&) -> Value {
        if (a.size() != 2) throw runtime_error("add expects 2 args");
        bool ai = std::holds_alternative<Int>(a[0]) || std::holds_alternative<bool>(a[0]);
        bool bi = std::holds_alternative<Int>(a[1]) || std::holds_alternative<bool>(a[1]);
        auto toI = [](const Value& v)->Int {
            if (std::holds_alternative<Int>(v)) return std::get<Int>(v);
            if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? 1 : 0;
            throw runtime_error("Expected integer-like");
        };
        if (ai && bi) return Value{ toI(a[0]) + toI(a[1]) };
        return Value{ as_number(a[0]) + as_number(a[1]) };
    });
    env.set_builtin("mul", [](const vector<Value>& a, Environment&) -> Value {
        if (a.size() != 2) throw runtime_error("mul expects 2 args");
        bool ai = std::holds_alternative<Int>(a[0]) || std::holds_alternative<bool>(a[0]);
        bool bi = std::holds_alternative<Int>(a[1]) || std::holds_alternative<bool>(a[1]);
        auto toI = [](const Value& v)->Int {
            if (std::holds_alternative<Int>(v)) return std::get<Int>(v);
            if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? 1 : 0;
            throw runtime_error("Expected integer-like");
        };
        if (ai && bi) return Value{ toI(a[0]) * toI(a[1]) };
        return Value{ as_number(a[0]) * as_number(a[1]) };
    });
    env.set_builtin("div", [](const vector<Value>& a, Environment&) -> Value {
        if (a.size() != 2) throw runtime_error("div expects 2 args");
        const double denom = as_number(a[1]);
        if (denom == 0.0) throw runtime_error("Division by zero");
        return Value{ as_number(a[0]) / denom };
    });
    env.set_builtin("print", [](const vector<Value>& a, Environment&) -> Value {
        for (size_t i = 0; i < a.size(); ++i) {
            cout << toString_value(a[i]);
            if (i + 1 < a.size()) cout << " ";
        }
        cout << endl;
        return a.empty() ? Value{ Int(0) } : a.back();
    });
    env.set_builtin("search", [](const vector<Value>& a, Environment&) -> Value {
        if (a.size() != 1) throw runtime_error("search expects 1 arg");
        return a[0];
    });

    // record(path, [append=false]) : start logging REPL input lines to file
    env.set_builtin("record", [](const vector<Value>& a, Environment&) -> Value {
        if (a.size() < 1 || a.size() > 2 || !std::holds_alternative<string>(a[0]))
            throw runtime_error("record expects (path, [append=false])");
        const string path = std::get<string>(a[0]);
        bool append = (a.size() == 2) ? (as_number(a[1]) != 0.0) : false;
        if (Recorder::out.is_open()) Recorder::out.close();
        Recorder::out.open(path, append ? (std::ios::app | std::ios::binary)
            : (std::ios::trunc | std::ios::binary));
        if (!Recorder::out) throw runtime_error("Failed to open record file: " + path);
        Recorder::enabled = true;
        return true;
    });

    env.set_builtin("record_off", [](const vector<Value>&, Environment&) -> Value {
        if (Recorder::out.is_open()) Recorder::out.close();
        Recorder::enabled = false;
        return true;
    });

    // script(path) : run a file with same env (multi-line ok if file is complete)
    env.set_builtin("script", [](const vector<Value> &a, Environment &env) -> Value {
        if (a.size() != 1 || !std::holds_alternative<string>(a[0]))
            throw runtime_error("script expects 1 string arg");
        const string path = std::get<string>(a[0]);
        std::ifstream in(path, std::ios::binary);
        if (!in) throw runtime_error("Failed to open script file: " + path);

        // read whole file and execute
        std::ostringstream ss; ss << in.rdbuf();
        AST script_tree;
        return script_tree.run(ss.str(), env); 
    });

    // env_export(path) / env_import(path) : save/load global variables
    env.set_builtin("env_export", [](const vector<Value>& a, Environment& env)->Value {
        if (a.size() != 1 || !std::holds_alternative<string>(a[0]))
            throw runtime_error("env_export expects 1 string arg");
        env.export_globals_to_file(std::get<string>(a[0]));
        return true;
    });
    env.set_builtin("env_import", [](const vector<Value>& a, Environment& env)->Value {
        if (a.size() != 1 || !std::holds_alternative<string>(a[0]))
            throw runtime_error("env_import expects 1 string arg");
        env.import_globals_from_file(std::get<string>(a[0]));
        return true;
    });
}



// ========================= REPL =========================
static void repl() {
    Environment env; register_builtins(env);

    AST ast;
    cout << "TVIZ CLI (type 'quit' to exit)" << endl;
    string line, acc;
    auto needs_more = [](const string& buf)->bool {
        int round = 0, curly = 0, square = 0;
        bool in_str = false, esc = false;
        for (char c : buf) {
            if (in_str) { if (esc) { esc = false; continue; } if (c == '\\') { esc = true; continue; } if (c == '"') in_str = false; continue; }
            if (c == '"') { in_str = true; continue; }
            if (c == '(') ++round; if (c == ')') --round;
            if (c == '{') ++curly; if (c == '}') --curly;
            if (c == '[') ++square; if (c == ']') --square;
        }
        if (in_str || esc) return true;
        if (round > 0 || curly > 0 || square > 0) return true;
        for (int i = (int)buf.size() - 1; i >= 0; --i) {
            char c = buf[i];
            if (std::isspace((unsigned char)c)) continue;
            return !(c == ';' || c == '}');
        }
        return true;
    };

    while (true) {
        cout << (acc.empty() ? ">> " : ".. ");
        if (!std::getline(cin, line)) break;
        if (acc.empty() && (line == "quit" || line == "exit")) break;
        
        acc += line + "\n";
        if (needs_more(acc)) continue;

        bool was_recorder_enabled = Recorder::enabled;

        ast.run(acc, env);
        
        if (was_recorder_enabled && Recorder::enabled && Recorder::out) {
            Recorder::out << line << "\n";
            Recorder::out.flush();
        }
        acc.clear();
    }
}