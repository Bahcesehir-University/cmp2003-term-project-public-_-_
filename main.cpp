#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <chrono>

using namespace std;

static inline void trim_inplace(string& s) {
    size_t a = 0;
    while (a < s.size() && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) a++;
    size_t b = s.size();
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) b--;
    s = s.substr(a, b - a);
}

bool split_6_auto(const string& line, vector<string>& cols) {
    auto try_split = [&](char delim) -> bool {
        cols.clear();
        cols.reserve(6);
        string item;
        stringstream ss(line);
        while (getline(ss, item, delim)) {
            trim_inplace(item);
            cols.push_back(item);
        }
        return cols.size() == 6;
    };

    if (try_split(',')) return true;
    if (try_split('\t')) return true;
    if (try_split(';')) return true;
    return false;
}

bool extract_hour(const string& dt, int& hour) {
    size_t colon = dt.find(':');
    if (colon == string::npos || colon < 2) return false;

    char c1 = dt[colon - 2];
    char c2 = dt[colon - 1];
    if (c1 < '0' || c1 > '9' || c2 < '0' || c2 > '9') return false;

    hour = (c1 - '0') * 10 + (c2 - '0');
    return hour >= 0 && hour <= 23;
}

static bool open_input(ifstream& file, const string& preferred) {
    file.open(preferred);
    if (file.is_open()) return true;
    file.clear();

    file.open("Trips.csv");
    if (file.is_open()) return true;
    file.clear();

    file.open("trips.csv");
    if (file.is_open()) return true;
    file.clear();

    return false;
}

int main(int argc, char* argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    auto start = chrono::high_resolution_clock::now();

    string path = (argc >= 2 ? string(argv[1]) : string("Trips.csv"));

    ifstream file;
    if (!open_input(file, path)) {
        cout << "Top 10 Pickup Zones:\n\n";
        cout << "Top 10 Busy Slots:\n\n";
        cout << "Execution time (seconds): 0\n";
        return 0;
    }

    unordered_map<string, long long> zone_total;
    unordered_map<string, array<long long, 24>> zone_hour;

    string line;
    vector<string> cols;

    bool header_checked = false;

    while (getline(file, line)) {
        if (!split_6_auto(line, cols)) continue;

        if (!header_checked) {
            header_checked = true;
            if (!cols.empty() && (cols[0] == "TripID" || cols[1] == "PickupZoneID")) continue;
        }

        string pickupZone = cols[1];
        string pickupDateTime = cols[3];

        if (pickupZone.empty()) continue;

        int hour;
        if (!extract_hour(pickupDateTime, hour)) continue;

        zone_total[pickupZone]++;

        auto it = zone_hour.find(pickupZone);
        if (it == zone_hour.end())
            it = zone_hour.emplace(pickupZone, array<long long, 24>{}).first;

        it->second[hour]++;
    }

    vector<pair<string, long long>> topZones;
    topZones.reserve(zone_total.size());
    for (const auto& p : zone_total) topZones.push_back(p);

    sort(topZones.begin(), topZones.end(),
         [](const auto& a, const auto& b) {
             if (a.second != b.second) return a.second > b.second;
             return a.first < b.first;
         });

    cout << "Top 10 Pickup Zones:\n";
    for (size_t i = 0; i < topZones.size() && i < 10; i++) {
        cout << topZones[i].first << " => " << topZones[i].second << "\n";
    }
    cout << "\n";

    struct Slot { string zone; int hour; long long count; };
    vector<Slot> slots;
    slots.reserve(zone_hour.size() * 24);

    for (const auto& p : zone_hour) {
        for (int h = 0; h < 24; h++) {
            long long c = p.second[h];
            if (c > 0) slots.push_back({p.first, h, c});
        }
    }

    sort(slots.begin(), slots.end(),
         [](const Slot& a, const Slot& b) {
             if (a.count != b.count) return a.count > b.count;
             if (a.zone != b.zone) return a.zone < b.zone;
             return a.hour < b.hour;
         });

    cout << "Top 10 Busy Slots:\n";
    for (size_t i = 0; i < slots.size() && i < 10; i++) {
        cout << slots[i].zone << " @ " << slots[i].hour << " => " << slots[i].count << "\n";
    }
    cout << "\n";

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;
    cout << "Execution time (seconds): " << elapsed.count() << "\n";

    return 0;
}
