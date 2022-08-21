#include "ASM/arm/ArmPostOptimizer.hh"
#include <cassert>
#include "Utility.hh"

static bool StartWith(const std::string &str, const char *substr) {
  size_t i = 0;
  while (*substr != 0 && i < str.size()) {
    if (*substr != str.at(i)) {
      return false;
    }
    ++i;
    ++substr;
  }
  //子串check完，则认为是完全包含的
  if (*substr == 0) {
    return true;
  }
  return false;
}

static bool StartWith(const std::string &str, const std::string &substr) {
  if (str.size() < substr.size()) {
    return false;
  }
  if (str.size() == substr.size()) {
    return str == substr;
  }
  return StartWith(str, substr.c_str());
}

//在set中从start开始选num个数放进buf，不会对buf初始化
static void FillBuffer(const std::set<size_t> set, std::vector<size_t> &buf, size_t start, size_t num) {
  if (set.size() < num) {
    throw std::logic_error("Not enough index in the set ( only " + std::to_string(set.size()) + " , but " +
                           std::to_string(num) + " required )");
  }
  auto it = set.lower_bound(start);
  int cur_num = num;
  while (cur_num > 0 && it != set.end()) {
    cur_num--;
    buf.push_back(*it);
    ++it;
  }
  if (cur_num > 0) {
    throw std::logic_error("Not enough index in the set start at " + std::to_string(start) + " ( only " +
                           std::to_string(num - cur_num) + " , but " + std::to_string(num) + " required )");
  }
}

static bool StartWithRegular(const std::vector<std::string> &lines, const std::vector<std::string> &regular,
                             std::vector<size_t> buf) {
  assert(regular.size() == buf.size());
  for (size_t i = 0; i < regular.size(); i++) {
    if (!StartWith(lines[buf[i]], regular[i])) {
      return false;
    }
  }
  return true;
}

static void SplitInst(std::vector<std::string> &result, const std::string &partial_inst) {
  std::string::size_type pos = 0;
  std::string::size_type prev = 0;
  while (true) {
    while (prev < partial_inst.size() && (partial_inst[prev] == ' ' || partial_inst[prev] == ',')) {
      prev++;
    }
    for (pos = prev; pos < partial_inst.size(); pos++) {
      if (partial_inst[pos] == ' ' || partial_inst[pos] == ',') {
        break;
      }
    }
    if (pos >= partial_inst.size()) {
      break;
    }
    result.push_back(partial_inst.substr(prev, pos - prev));
    prev = pos + 1;
  }
  if (prev < partial_inst.size()) {
    result.push_back(partial_inst.substr(prev));
  }
}

namespace HaveFunCompiler {
namespace AssemblyBuilder {
ArmPostOptimizer::ArmPostOptimizer(const std::string &arm_assembly)
    : is_optimized(false), arm_assembly_(arm_assembly) {}

const std::string &ArmPostOptimizer::optimize() {
  if (!is_optimized) {
    OptimizeImpl();
    is_optimized = true;
  }
  return optimized_;
}

void ArmPostOptimizer::OptimizeImpl() {
  //分割成单行
  SplitToLines();
  //初始化is_deleted_
  is_deleted_.resize(lines_.size(), false);
  //集中所有非注释行
  SummarizeNoncommentLines();

  //按顺序做效果会好些
  //合并取反
  SubOptimize1();
  //跳转与条件合并
  SubOptimize2();

  optimized_.clear();
  for (size_t i = 0; i < lines_.size(); i++) {
    if (is_deleted_[i]) {
      continue;
    }
    optimized_.append(lines_[i]);
    optimized_.append("\n");
  }
}

void ArmPostOptimizer::SplitToLines() {
  lines_.clear();
  std::string::size_type pos = 0;
  std::string::size_type prev = 0;
  while ((pos = arm_assembly_.find('\n', prev)) != std::string::npos) {
    lines_.push_back(arm_assembly_.substr(prev, pos - prev));
    prev = pos + 1;
  }
  lines_.push_back(arm_assembly_.substr(prev));
  //去除空行
  EraseIf(lines_, [](const std::string &str) -> bool {
    for (auto c : str) {
      if (c == '\n' || c == '\r' || c == ' ') {
        continue;
      }
      return false;
    }
    return true;
  });
  //去除每行行前行末空格
  for (auto &line : lines_) {
    size_t i;
    for (i = 0; i < line.size(); i++) {
      if (line[i] == ' ') {
        continue;
      }
      break;
    }
    line = line.substr(i);
    if (!line.empty()) {
      i = line.size() - 1;
      while (i > 0 && line[i] == ' ') {
        i--;
      }
      line = line.substr(0, i + 1);
    }
  }
}

void ArmPostOptimizer::SummarizeNoncommentLines() {
  noncomm_lines_idx_.clear();
  for (size_t i = 0; i < lines_.size(); i++) {
    if (StartWith(lines_[i], "//")) {
      continue;
    }
    noncomm_lines_idx_.insert(i);
  }
}
void ArmPostOptimizer::DeleteLine(size_t line_idx) {
  assert(line_idx < lines_.size());
  noncomm_lines_idx_.erase(line_idx);
  is_deleted_[line_idx] = true;
}

void ArmPostOptimizer::SubOptimize2() {
  /*
  优化形如
  cmp r6, #0
  moveq r5, #1
  movne r5, #0
  cmp r5, #0
  beq S2SL_3
  的语句，使其变为
  cmp r6, #0
  bne S2SL_3
  (合并if)
  */
  //至少需求5句
  if (noncomm_lines_idx_.size() < 5) {
    return;
  }
  std::vector<size_t> buf;
  ssize_t res_siz = noncomm_lines_idx_.size() - 5;

  //注意有空格，是特意为之。
  const static std::vector<std::string> regular = {"cmp ", "mov", "mov", "cmp ", "beq "};

  const static std::vector<std::string> condition_regular = {"moveq ", "movlt ", "movgt ",
                                                             "movne ", "movge ", "movle "};

  const static std::vector<std::string> changeto = {"ne", "ge", "le", "eq", "lt", "gt"};

  FillBuffer(noncomm_lines_idx_, buf, 0, 5);

  do {
    if (StartWithRegular(lines_, regular, buf) && CheckCanOptimize(buf)) {
      ssize_t condition_test_res = -1;
      for (size_t i = 0; i < condition_regular.size(); i++) {
        if (StartWith(lines_[buf[1]], condition_regular[i]) &&
            StartWith(lines_[buf[2]], condition_regular[(i + 3) % 6])) {
          //相反的两个条件mov
          condition_test_res = i;
          break;
        }
      }
      if (condition_test_res != -1) {
        std::vector<std::vector<std::string>> inst_part;
        //需要对中间三句进行判定
        inst_part.resize(3);
        SplitInst(inst_part[0], lines_[buf[1]].substr(std::string("movxx ").length()));
        SplitInst(inst_part[1], lines_[buf[2]].substr(std::string("movxx ").length()));
        SplitInst(inst_part[2], lines_[buf[3]].substr(std::string("cmp ").length()));
        bool inst_part_size_test = true;
        for (int i = 0; i < 3; i++) {
          if (inst_part[i].size() != 2) {
            inst_part_size_test = false;
            break;
          }
        }
        if (inst_part_size_test) {
          if (inst_part[1][0] == inst_part[0][0] && inst_part[2][0] == inst_part[0][0]) {
            if (((inst_part[0][1] == "#1" && inst_part[1][1] == "#0") ||
                 (inst_part[0][1] == "#0" && inst_part[1][1] == "#1")) &&
                inst_part[2][1] == "#0") {
              //通过测试，可以变更
              //删除中间两句
              DeleteLine(buf[1]);
              DeleteLine(buf[2]);
              DeleteLine(buf[3]);
              if (inst_part[0][1] == "#0") {
                //如果赋值是相反的，结果也反向
                condition_test_res = (condition_test_res + 3) % 6;
              }
              lines_[buf[4]][1] = changeto[condition_test_res][0];
              lines_[buf[4]][2] = changeto[condition_test_res][1];
              if (res_siz < 5) {
                break;
              }
              res_siz -= 5;
              size_t start = buf.back() + 1;
              buf.clear();
              FillBuffer(noncomm_lines_idx_, buf, start, 5);
              continue;
            }
          }
        }
      }
    }
    if (res_siz == 0) {
      break;
    }
    res_siz--;
    buf.erase(buf.begin());
    FillBuffer(noncomm_lines_idx_, buf, buf.back() + 1, 1);
  } while (res_siz > 0);
}

bool ArmPostOptimizer::CheckCanOptimize(const std::vector<size_t> &buf) {
  size_t l = buf[0];
  size_t r = buf.back();
  const static size_t FIND_RANGE = 6;
  size_t res;
  res = FIND_RANGE;
  for (size_t i = l; i <= r; i++) {
    if (lines_[i].find("CommExpGen_") != lines_[i].npos) {
      return false;
    }
  }

  while (res > 0) {
    if (StartWith(lines_[l], "//")) {
      if (lines_[l].find("CommExpGen_") != lines_[l].npos) {
        return false;
      }
      res--;
    }
    if (l == 0) {
      break;
    }
    l--;
  }
  res = FIND_RANGE;
  while (r < lines_.size() && res > 0) {
    if (StartWith(lines_[r], "//")) {
      if (lines_[r].find("CommExpGen_") != lines_[r].npos) {
        return false;
      }
      res--;
    }
    r++;
    if (r >= lines_.size()) {
      break;
    }
  }
  return true;
}

void ArmPostOptimizer::SubOptimize1() {
  /*
  优化形如
  cmp r7, r6
  movgt r8, #1
  movle r8, #0
  cmp r8, #0
  moveq r5, #1
  movne r5, #0
  的语句为
  cmp r7, r6
  movgt r5, #0
  movgt r5, #1
  (合并取反!)
  */
  //至少需要6句
  if (noncomm_lines_idx_.size() < 6) {
    return;
  }
  std::vector<size_t> buf;
  ssize_t res_siz = noncomm_lines_idx_.size() - 6;

  //注意有空格，是特意为之。
  const static std::vector<std::string> regular = {"cmp ", "mov", "mov", "cmp ", "moveq ", "movne "};

  const static std::vector<std::string> condition_regular = {"moveq ", "movlt ", "movgt ",
                                                             "movne ", "movge ", "movle "};

  const static std::vector<std::string> changeto = {"ne", "ge", "le", "eq", "lt", "gt"};

  FillBuffer(noncomm_lines_idx_, buf, 0, 6);

  do {
    if (StartWithRegular(lines_, regular, buf) && CheckCanOptimize(buf)) {
      ssize_t condition_test_res = -1;
      for (size_t i = 0; i < condition_regular.size(); i++) {
        if (StartWith(lines_[buf[1]], condition_regular[i]) &&
            StartWith(lines_[buf[2]], condition_regular[(i + 3) % 6])) {
          //相反的两个条件mov
          condition_test_res = i;
          break;
        }
      }
      if (condition_test_res != -1) {
        std::vector<std::vector<std::string>> inst_part;
        //需要对后五句进行判定
        inst_part.resize(5);
        SplitInst(inst_part[0], lines_[buf[1]].substr(std::string("movxx ").length()));
        SplitInst(inst_part[1], lines_[buf[2]].substr(std::string("movxx ").length()));
        SplitInst(inst_part[2], lines_[buf[3]].substr(std::string("cmp ").length()));
        SplitInst(inst_part[3], lines_[buf[4]].substr(std::string("movxx ").length()));
        SplitInst(inst_part[4], lines_[buf[5]].substr(std::string("movxx ").length()));
        bool inst_part_size_test = true;
        for (int i = 0; i < 5; i++) {
          if (inst_part[i].size() != 2) {
            inst_part_size_test = false;
            break;
          }
        }
        if (inst_part_size_test) {
          if (inst_part[1][0] == inst_part[0][0] && inst_part[2][0] == inst_part[0][0] &&
              inst_part[3][0] == inst_part[4][0]) {
            if (inst_part[0][1] == "#1" && inst_part[1][1] == "#0" && inst_part[2][1] == "#0" &&
                ((inst_part[3][1] == "#1" && inst_part[4][1] == "#0") ||
                 (inst_part[3][1] == "#0" && inst_part[4][1] == "#1"))) {
              //通过测试，可以变更
              DeleteLine(buf[3]);
              DeleteLine(buf[4]);
              DeleteLine(buf[5]);
              lines_[buf[1]] = lines_[buf[1]].substr(0, std::string("movxx").length()) + " " + inst_part[3][0] + ", ";
              lines_[buf[2]] = lines_[buf[2]].substr(0, std::string("movxx").length()) + " " + inst_part[4][0] + ", ";
              if (inst_part[3][1] == "#1") {
                lines_[buf[1]] += "#0";
                lines_[buf[2]] += "#1";
              } else {
                lines_[buf[1]] += "#1";
                lines_[buf[2]] += "#0";
              }
              if (res_siz < 6) {
                break;
              }
              res_siz -= 6;
              size_t start = buf.back() + 1;
              buf.clear();
              FillBuffer(noncomm_lines_idx_, buf, start, 6);
              continue;
            }
          }
        }
      }
    }
    if (res_siz == 0) {
      break;
    }
    res_siz--;
    buf.erase(buf.begin());
    FillBuffer(noncomm_lines_idx_, buf, buf.back() + 1, 1);
  } while (res_siz > 0);
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler