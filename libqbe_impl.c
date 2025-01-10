#include "libqbe.h"

typedef struct QQLinkageData {
  bool export;
  bool tls;
  bool common;
  const char* section_name;
  const char* section_flags;
} QQLinkageData;

QQData qq_begin_data(const char* name, QQLinkage linkage) {
  return (QQData){0};
}
