#pragma once

// Cleans Persian transcript text: Arabic yeh/kaf and digits -> Persian forms, and
// zero-width non-joiners for «می‌», «‌ها» and «ه‌ای». Returns a malloc'ed string.
char *persian_fix(const char *text);
