#include "qbelib.h"

typedef struct QQLinkageData {
  bool export;
  bool tls;
  bool common;
  const char* section_name;
  const char* section_flags;
} QQLinkageData;

QQData qq_begin_data(const char* name, QQLinkage linkage) {
#if 0
  d.type = DStart;
  d.name = name;
  d.lnk = lnk;
  cb(&d);
#endif
  return (QQData){0};
}
