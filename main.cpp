#include <iostream>
#include <thread>
#include <fstream>
#include <string>
#include <filesystem>
#include <tuple>
#include <algorithm>
#include <chrono>
#include <regex>

#include <boost/multiprecision/cpp_int.hpp>

using namespace std;

namespace mp = boost::multiprecision;
namespace fs = std::filesystem;

const auto iter = 1 + 28;
const string resultsDirectory = "results";


void saveResult(const mp::cpp_int &power, int num) {
    vector<unsigned char> buffer;
    export_bits(power, back_inserter(buffer), 8);

    fstream file(resultsDirectory + "/" + to_string(num) + ".txt", fstream::out | fstream::trunc);
    for (unsigned char val: buffer) {
        file.put(val);
    }
    file.close();
}

void saveResult(const mp::cpp_int &power, const string &filename) {
    vector<unsigned char> buffer;
    export_bits(power, back_inserter(buffer), 8);

    fstream file(resultsDirectory + "/" + filename + ".txt", fstream::out | fstream::trunc);
    for (unsigned char val: buffer) {
        file.put(val);
    }
    file.close();
}

tuple <mp::cpp_int*, int> loadResults() {
    auto *powers = new mp::cpp_int[iter];
    mp::cpp_int partial;
    vector<int> loadedPowers;
    auto numberToLarge = false;

    for (const auto &entry : fs::directory_iterator(resultsDirectory)) {
        auto filename = fs::path(entry).stem(); // Get filename without path and extension
        int power = atoi(filename.c_str()); // Convert string into integer

        if (power == 0) {
            continue;
        }

        if (power < iter) {
            cout << "Loading " + entry.path().filename().string() << endl;

            vector<unsigned char> buffer;
            fstream file(entry.path(), fstream::in);

            while (!file.eof()) {
                char val;
                file.get(val);
                if (file.eof()) break;
                buffer.push_back(val);
            }
            file.close();

            import_bits(powers[power], buffer.begin(), buffer.end());

            if (powers[power] == 0) {
                continue;
            }
            loadedPowers.push_back(power); // Add read power to loadedPowers
        } else {
            numberToLarge = true;
        }
    }

    cout << "Finished Loading" << endl;
    if (numberToLarge) {
        cout << "Found file with power larger than set possible!" << endl << endl;
    }

    int largestPower = 0;
    sort(loadedPowers.begin(), loadedPowers.end());

    // Find largest power without "gaps" in files, so
    // 0, 1, 2, 3, 5, 6 will return 3
    // We will need all powers in stage two
    for (auto power: loadedPowers) {
        if (power == largestPower + 1){
            largestPower++;
        }
    }

    powers[0] = 9;
    largestPower = max(++largestPower, 1); // Set largestPower to bigger value: 1 or largestPower + 1

    return {powers, largestPower};
}

string prepareTime(chrono::steady_clock::time_point start, chrono::steady_clock::time_point end) {
    auto microseconds = chrono::duration_cast<chrono::microseconds>(end - start).count();
    auto milliseconds = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    auto seconds = chrono::duration_cast<chrono::seconds>(end - start).count();
    auto hours = chrono::duration_cast<chrono::hours>(end - start).count();

    string time;

    if (hours > 0) {
        time += to_string(hours) + "h ";
    }
    if (seconds > 0) {
        time += to_string(seconds % 60) + "s ";
    }
    if (milliseconds > 0) {
        time += to_string(milliseconds % 1000) + "ms ";
    }
    if (microseconds > 0) {
        time += to_string(microseconds % 1000) + "μm";
    }

    if (time.empty()) {
        return "no time";
    } else {
        return time;
    }
}

int main() {
    auto [powers, currentPower] = loadResults();
    thread threads[iter];

    for (; currentPower < iter; ++currentPower) {
        auto startTime = chrono::steady_clock::now();

        powers[currentPower] = pow(powers[currentPower - 1], 2);

        auto endTime = chrono::steady_clock::now();
        cout << "Calculated 9^" << mp::pow(mp::cpp_int(2), currentPower) << ", in " << prepareTime(startTime, endTime) << ", step " << currentPower << endl; // For logging progress

        // thread() can't find rsquare prismight function by its own (there are two),
        // so I point it to right address manually
        auto functionAddress = static_cast<void(*)(const mp::cpp_int&, int)>(saveResult);
        threads[currentPower] = thread(functionAddress, powers[currentPower], currentPower);
    }

    // Stage two: multiply following elements from array:
    // int toMultiply[] =  {28, 26, 25, 24, 20, 18, 17, 16, 15, 12, 8, 6, 3, 0};
    int toMultiply[] = {3, 6, 8, 12, 15, 16, 17, 18, 20, 24, 25, 26, 28};
    // Based on:
    // https://www.wolframalpha.com/input/?i=9%5E9+%3D+2%5E28+%2B+2%5E26+%2B+2%5E25+%2B+2%5E24+%2B+2%5E20+%2B+2%5E18+%2B+2%5E17+%2B+2%5E16+%2B+2%5E15+%2B+2%5E12+%2B+2%5E8+%2B+2%5E6+%2B+2%5E3+%2B+2%5E0

    mp::cpp_int final = powers[0];
    int previous = 0;
    for (int i = 0; i < 13; i++) {
        cout << "Calculating: " << toMultiply[i] << endl;
        final *= powers[toMultiply[i]];

        cout << "Saving..." << endl;
        saveResult(final, "partial." + to_string(toMultiply[i]));
        remove(("partial." + to_string(previous)).c_str());
    }

    for (auto &th: threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    fstream result("result.txt", fstream::out);
    result << final;
    result.close();
    return 0;
}