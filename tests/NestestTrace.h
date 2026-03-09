#ifndef NESTEST_TRACE_H
#define NESTEST_TRACE_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>

#include "../include/NES.h"

// written by LLM for comparison of NEStest log outputs

namespace nestest {

inline std::filesystem::path testDataPath(const char *filename) {
    return std::filesystem::path(__FILE__).parent_path() / filename;
}

inline std::vector<uint8_t> readBinaryFile(const char *filename) {
    std::ifstream file(testDataPath(filename), std::ios::binary);
    if (!file) {
        throw std::runtime_error(std::string("Could not open file: ") +
                                 filename);
    }

    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
}

inline std::vector<std::string> readLines(const char *filename) {
    std::ifstream file(testDataPath(filename));
    if (!file) {
        throw std::runtime_error(std::string("Could not open file: ") +
                                 filename);
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

inline std::string swapPpuFields(std::string line) {
    const size_t ppuPos = line.find("PPU:");
    if (ppuPos == std::string::npos) {
        return line;
    }

    const size_t commaPos = line.find(',', ppuPos);
    const size_t cycPos = line.find(" CYC:", ppuPos);
    if (commaPos == std::string::npos || cycPos == std::string::npos) {
        return line;
    }

    const std::string firstField =
        line.substr(ppuPos + 4, commaPos - (ppuPos + 4));
    const std::string secondField =
        line.substr(commaPos + 1, cycPos - (commaPos + 1));
    line.replace(ppuPos + 4, cycPos - (ppuPos + 4),
                 secondField + "," + firstField);
    return line;
}

class LineCompareStreamBuf : public std::streambuf {
  public:
    explicit LineCompareStreamBuf(const std::vector<std::string> &expectedLines)
        : expectedLines(expectedLines) {}

    bool failed() const { return !failureMessage.empty(); }
    bool done() const { return lineIndex == expectedLines.size(); }
    size_t linesCompared() const { return lineIndex; }
    const std::string &failure() const { return failureMessage; }

  protected:
    int overflow(int ch) override {
        if (ch == traits_type::eof()) {
            return traits_type::not_eof(ch);
        }

        const char c = static_cast<char>(ch);
        if (c == '\r') {
            return ch;
        }

        if (c == '\n') {
            compareCurrentLine();
            currentLine.clear();
            return ch;
        }

        currentLine.push_back(c);
        return ch;
    }

    int sync() override {
        if (!currentLine.empty()) {
            compareCurrentLine();
            currentLine.clear();
        }
        return failed() ? -1 : 0;
    }

  private:
    void compareCurrentLine() {
        if (failed()) {
            return;
        }

        if (lineIndex >= expectedLines.size()) {
            failureMessage = "Logger produced more lines than expected";
            return;
        }

        const std::string &expected = expectedLines[lineIndex];
        if (currentLine != expected) {
            failureMessage =
                "Mismatch at line " + std::to_string(lineIndex + 1) +
                "\nexpected: " + expected + "\nactual:   " + currentLine;
            return;
        }

        lineIndex++;
    }

    const std::vector<std::string> &expectedLines;
    std::string currentLine;
    std::string failureMessage;
    size_t lineIndex = 0;
};

class ScopedStreamCapture {
  public:
    explicit ScopedStreamCapture(std::streambuf *replacement)
        : original(std::cout.rdbuf(replacement)) {}

    ~ScopedStreamCapture() { std::cout.rdbuf(original); }

  private:
    std::streambuf *original;
};

class CpuPpuStepper {
  public:
    explicit CpuPpuStepper(NES &nes) : nes(nes) {}

    void tick() {
        nes.cpu.tick();

        if (pendingNMIEdge) {
            nes.cpu.triggerNMI();
            pendingNMIEdge = false;
        }

        for (int i = 0; i < 3; i++) {
            nes.ppu.tick();
            const bool nmiState = nes.bus.ppuNMI();
            if (nmiState && !lastNMIState) {
                if (nes.cpu.completedTakenBranchLastTick()) {
                    pendingNMIEdge = true;
                } else {
                    nes.cpu.triggerNMI();
                }
            }
            lastNMIState = nmiState;
        }
    }

  private:
    NES &nes;
    bool lastNMIState = false;
    bool pendingNMIEdge = false;
};

} // namespace nestest

#endif // NESTEST_TRACE_H
