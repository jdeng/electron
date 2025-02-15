// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/session_preferences.h"

namespace electron {

// static
int SessionPreferences::kLocatorKey = 0;

SessionPreferences::SessionPreferences(content::BrowserContext* context) {
  context->SetUserData(&kLocatorKey, base::WrapUnique(this));
}

SessionPreferences::~SessionPreferences() = default;

// static
SessionPreferences* SessionPreferences::FromBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SessionPreferences*>(context->GetUserData(&kLocatorKey));
}

// static
std::vector<base::FilePath::StringType> SessionPreferences::GetValidPreloads(
    content::BrowserContext* context) {
  std::vector<base::FilePath::StringType> result;

  if (auto* self = FromBrowserContext(context)) {
    for (const auto& preload : self->preloads()) {
      if (base::FilePath(preload).IsAbsolute()) {
        result.emplace_back(preload);
      } else {
        LOG(ERROR) << "preload script must have absolute path: " << preload;
      }
    }
  }

  return result;
}

}  // namespace electron
