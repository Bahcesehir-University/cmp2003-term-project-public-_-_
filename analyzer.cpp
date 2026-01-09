#include "analyzer.h"
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

static inline void trim_inplace(string& s) {
    size_t a = 0;
    while (a < s.size() && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) a++;
    size_t b = s.size();
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) b--;
    s = s.substr(a, b - a);
}

static bool split6_comma(const string& line, vector<string>& cols) {
    cols.clear();
    cols.reserve(6);
    string item;
    stringstream ss(line);
    while (getline(ss, item, ',')) {
        trim_inplace(item);
        cols.push_back(item);
    }
    return cols.size() == 6;
}

static bool extract_hour(const string& dt, int& hour) {
    size_t colon = dt.find(':');
    if (colon == string::npos || colon < 2) return false;

    char c1 = dt[colon - 2];
    char c2 = dt[colon - 1];
    if (c1 < '0' || c1 > '9' || c2 < '0' || c2 > '9') return false;

    hour = (c1 - '0') * 10 + (c2 - '0');
    return hour >= 0 && hour <= 23;
}

void TripAnalyzer::ingestFile(const std::string& csvPath) {
    zoneTotals.clear();
    zoneHourlySlots.clear();

    ifstream in(csvPath);
    if (!in.is_open()) return;

    string line;
    vector<string> cols;
    bool header_checked = false;

    while (getline(in, line)) {
        if (!split6_comma(line, cols)) continue;

        if (!header_checked) {
            header_checked = true;
            if (!cols.empty() && cols[0] == "TripID") continue;
        }

        const string& pickupZone = cols[1];
        const string& pickupDateTime = cols[3];

        if (pickupZone.empty()) continue;
        if (pickupDateTime.empty()) continue;

        int hour;
        if (!extract_hour(pickupDateTime, hour)) continue;

        zoneTotals[pickupZone]++;

        auto it = zoneHourlySlots.find(pickupZone);
        if (it == zoneHourlySlots.end()) {
            it = zoneHourlySlots.emplace(pickupZone, array<long long, 24>{}).first;
        }
        it->second[hour]++;
    }
}

std::vector<ZoneCount> TripAnalyzer::topZones(int k) const {
    vector<ZoneCount> v;
    v.reserve(zoneTotals.size());

    for (const auto& p : zoneTotals) {
        v.push_back({p.first, p.second});
    }

    sort(v.begin(), v.end(), [](const ZoneCount& a, const ZoneCount& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.zone < b.zone;
    });

    if (k < 0) k = 0;
    if ((int)v.size() > k) v.resize(k);
    return v;
}

std::vector<SlotCount> TripAnalyzer::topBusySlots(int k) const {
    vector<SlotCount> v;
    v.reserve(zoneHourlySlots.size() * 24);

    for (const auto& p : zoneHourlySlots) {
        const string& zone = p.first;
        for (int h = 0; h < 24; h++) {
            long long c = p.second[h];
            if (c > 0) v.push_back({zone, h, c});
        }
    }

    sort(v.begin(), v.end(), [](const SlotCount& a, const SlotCount& b) {
        if (a.count != b.count) return a.count > b.count;
        if (a.zone != b.zone) return a.zone < b.zone;
        return a.hour < b.hour;
    });

    if (k < 0) k = 0;
    if ((int)v.size() > k) v.resize(k);
    return v;
}
