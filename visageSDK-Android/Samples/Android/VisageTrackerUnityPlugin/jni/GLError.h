#ifndef GLERROR_H
#define GLERROR_H

void _check_gl_error(const char *file, int line, const char *msg);

///
/// Usage
/// [... some opengl calls]
/// glCheckError();
///
#define printGLError(MSG) _check_gl_error(__FILE__, __LINE__, MSG)

#endif // GLERROR_H