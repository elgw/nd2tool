#ifndef __nd2lib_h__
#define __nd2lib_h__

#include <stdlib.h>

typedef char                LIMCHAR;
typedef LIMCHAR*            LIMSTR;
typedef LIMCHAR const*      LIMCSTR;
typedef wchar_t             LIMWCHAR;
typedef LIMWCHAR*           LIMWSTR;
typedef LIMWCHAR const*     LIMCWSTR;
typedef unsigned int        LIMUINT;
typedef unsigned long long  LIMUINT64;
typedef size_t              LIMSIZE;
typedef int                 LIMINT;
typedef int                 LIMBOOL;
typedef int                 LIMRESULT;

#define LIM_OK                    0
#define LIM_ERR_UNEXPECTED       -1
#define LIM_ERR_NOTIMPL          -2
#define LIM_ERR_OUTOFMEMORY      -3
#define LIM_ERR_INVALIDARG       -4
#define LIM_ERR_NOINTERFACE      -5
#define LIM_ERR_POINTER          -6
#define LIM_ERR_HANDLE           -7
#define LIM_ERR_ABORT            -8
#define LIM_ERR_FAIL             -9
#define LIM_ERR_ACCESSDENIED     -10
#define LIM_ERR_OS_FAIL          -11
#define LIM_ERR_NOTINITIALIZED   -12
#define LIM_ERR_NOTFOUND         -13
#define LIM_ERR_IMPL_FAILED      -14
#define LIM_ERR_DLG_CANCELED     -15
#define LIM_ERR_DB_PROC_FAILED   -16
#define LIM_ERR_OUTOFRANGE       -17
#define LIM_ERR_PRIVILEGES       -18
#define LIM_ERR_VERSION          -19
#define LIM_SUCCESS(res)         (0 <= (res))

struct _LIMPICTURE
{
    LIMUINT  uiWidth;
    LIMUINT  uiHeight;
    LIMUINT  uiBitsPerComp;
    LIMUINT  uiComponents;
    LIMSIZE  uiWidthBytes;
    LIMSIZE  uiSize;
    void*    pImageData;
};

typedef struct _LIMPICTURE LIMPICTURE;
typedef void*  LIMFILEHANDLE;

#if defined(_WIN32) && defined(_DLL) && !defined(LX_STATIC_LINKING)
#  define DLLEXPORT __declspec(dllexport)
#  define DLLIMPORT __declspec(dllimport)
#else
#  define DLLEXPORT
#  define DLLIMPORT
#endif

#if defined(__cplusplus)
#  define EXTERN extern "C"
#else
#  define EXTERN extern
#endif

#if defined(GNR_ND2_SDK_EXPORTS)
#  define LIMFILEAPI EXTERN DLLEXPORT
#else
#  define LIMFILEAPI EXTERN DLLIMPORT
#endif

LIMFILEAPI LIMFILEHANDLE   Lim_FileOpenForRead(LIMCWSTR wszFileName);
LIMFILEAPI LIMFILEHANDLE   Lim_FileOpenForReadUtf8(LIMCSTR szFileNameUtf8);
LIMFILEAPI void            Lim_FileClose(LIMFILEHANDLE hFile);
LIMFILEAPI LIMSIZE         Lim_FileGetCoordSize(LIMFILEHANDLE hFile);
LIMFILEAPI LIMUINT         Lim_FileGetCoordInfo(LIMFILEHANDLE hFile,
                                                LIMUINT coord, LIMSTR type,
                                                LIMSIZE maxTypeSize);
LIMFILEAPI LIMUINT         Lim_FileGetSeqCount(LIMFILEHANDLE hFile);
LIMFILEAPI LIMBOOL         Lim_FileGetSeqIndexFromCoords(LIMFILEHANDLE hFile,
                                                         const LIMUINT * coords,
                                                         LIMSIZE coordCount,
                                                         LIMUINT* seqIdx);
LIMFILEAPI LIMSIZE         Lim_FileGetCoordsFromSeqIndex(LIMFILEHANDLE hFile,
                                                         LIMUINT seqIdx,
                                                         LIMUINT* coords,
                                                         LIMSIZE maxCoordCount);
LIMFILEAPI LIMSTR          Lim_FileGetAttributes(LIMFILEHANDLE hFile);
LIMFILEAPI LIMSTR          Lim_FileGetMetadata(LIMFILEHANDLE hFile);
LIMFILEAPI LIMSTR          Lim_FileGetFrameMetadata(LIMFILEHANDLE hFile,
                                                    LIMUINT uiSeqIndex);
LIMFILEAPI LIMSTR          Lim_FileGetTextinfo(LIMFILEHANDLE hFile);
LIMFILEAPI LIMSTR          Lim_FileGetExperiment(LIMFILEHANDLE hFile);
LIMFILEAPI LIMRESULT       Lim_FileGetImageData(LIMFILEHANDLE hFile,
                                                LIMUINT uiSeqIndex,
                                                LIMPICTURE* pPicture);
LIMFILEAPI void            Lim_FileFreeString(LIMSTR str);
LIMFILEAPI LIMSIZE         Lim_InitPicture(LIMPICTURE* pPicture, LIMUINT width,
                                           LIMUINT height, LIMUINT bpc,
                                           LIMUINT components);
LIMFILEAPI void            Lim_DestroyPicture(LIMPICTURE* pPicture);
#endif
