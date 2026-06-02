/**
 * @file scene_params.h
 * @brief Mitsuba <default> template-parameter substitution.
 * @author Alexei Czornyj
 * @date 2026
 */

#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace kestrel {

/**
 * @brief Substitute Mitsuba template parameters in an attribute value.
 *
 * Mitsuba scenes declare `<default name="spp" value="64"/>` and reference it as
 * `$spp` or `${spp}`. This replaces every such reference in `s` with the value
 * from `params`. Longer names are substituted first so a short name is never
 * matched inside a longer one (e.g. `$x` inside `$xy`). Unknown `$name`
 * references are left untouched.
 */
inline std::string substitute_params(std::string s,
                                     const std::map<std::string, std::string> &params) {
  if (params.empty() || s.find('$') == std::string::npos) return s;

  std::vector<const std::string *> keys;
  keys.reserve(params.size());
  for (const auto &kv : params) keys.push_back(&kv.first);
  std::sort(keys.begin(), keys.end(),
            [](const std::string *a, const std::string *b) { return a->size() > b->size(); });

  for (const std::string *kp : keys) {
    const std::string &v = params.at(*kp);
    for (const std::string &pat : {"${" + *kp + "}", "$" + *kp}) {
      size_t pos = 0;
      while ((pos = s.find(pat, pos)) != std::string::npos) {
        s.replace(pos, pat.size(), v);
        pos += v.size();
      }
    }
  }
  return s;
}

}  // namespace kestrel
