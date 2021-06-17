#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <unordered_map>

#include "bf/all.hpp"
#include "configuration.h"

using namespace util;
using namespace bf;

trial<nothing> run(config const& cfg) {
    auto numeric = cfg.check("numeric");
    auto k = *cfg.as<size_t>("hash-functions");
    auto cells = *cfg.as<size_t>("cells");
    // auto seed = *cfg.as<size_t>("seed");
    auto fpr = *cfg.as<double>("fp-rate");
    auto capacity = *cfg.as<size_t>("capacity");
    // auto part = cfg.check("partition");
    // auto double_hashing = cfg.check("double-hashing");

    auto const& type = *cfg.as<std::string>("type");
    std::unique_ptr<bloom_filter> bf;

    if (type == "basic") {
        if (fpr == 0 || capacity == 0) {
            if (cells == 0)
                return error{"need non-zero cells"};
            if (k == 0)
                return error{"need non-zero k"};

            // auto h = make_hasher(k);
            bf.reset(new basic_bloom_filter(std::move(k), cells));
        } else {
            assert(fpr != 0 && capacity != 0);
            bf.reset(make_filter_ptr(fpr, capacity));
        }
    } else {
        return error{"invalid bloom filter type"};
    }

    std::string line;
    auto input_file = *cfg.as<std::string>("input");
    std::ifstream in{input_file};
    if (!in)
        return error{"cannot read " + input_file};

    in >> std::noskipws;

    while (std::getline(in, line)) {
        if (line.empty())
            continue;

        auto p = line.data();
        while (*p)
            if (*p == ' ' || *p == '\t')
                return error{"whitespace in input not supported"};
            else
                ++p;

        if (numeric)
            bf->add(std::strtod(line.c_str(), nullptr));
        else
            bf->add(line);
    }

    size_t tn = 0, tp = 0, fp = 0, fn = 0;
    size_t ground_truth;
    std::string element;
    auto query_file = *cfg.as<std::string>("query");
    std::ifstream query{query_file};
    if (!query)
        return error{"cannot read " + query_file};

    std::cout << "TN TP FP FN G C E" << std::endl;
    while (query >> ground_truth >> element)  // uniq -c
    {
        size_t count;
        if (numeric)
            count = bf->lookup(std::strtod(element.c_str(), nullptr));
        else
            count = bf->lookup(element);

        if (!query)
            return error{"failed to parse element"};

        if (count == 0 && ground_truth == 0)
            ++tn;
        else if (count == ground_truth)
            ++tp;
        else if (count > ground_truth)
            ++fp;
        else
            ++fn;

        std::cout << tn << ' ' << tp << ' ' << fp << ' ' << fn << ' '
                  << ground_truth << ' ' << count << ' ';

        if (numeric)
            std::cout << std::strtod(element.c_str(), nullptr);
        else
            std::cout << element;

        std::cout << std::endl;
    }

    return nil;
}

int main(int argc, char* argv[]) {
    auto cfg = config::parse(argc, argv);
    if (!cfg) {
        std::cerr << cfg.failure().msg() << ", try -h or --help" << std::endl;
        return 1;
    }

    if (argc < 2 || cfg->check("help") || cfg->check("advanced")) {
        cfg->usage(std::cerr, cfg->check("advanced"));
        return 0;
    }

    if (!cfg->check("type")) {
        std::cerr << "missing bloom filter type" << std::endl;
        return 1;
    }

    if (!cfg->check("input")) {
        std::cerr << "missing input file" << std::endl;
        return 1;
    }

    if (!cfg->check("query")) {
        std::cerr << "missing query file" << std::endl;
        return 1;
    }

    auto t = run(*cfg);
    if (!t) {
        std::cerr << t.failure().msg() << std::endl;
        return 1;
    }

    return 0;
}
