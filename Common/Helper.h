//===================================================================//
//   Beat Tracking Alogrithm                                         //
//   Implemented on Software for Microsoft® Windows®                 //
//                                                                   //
//   Team of Sharif University of Technology - IRAN                  //
//                                                                   //
//   IEEE Signal Processing Cup 2017 - Beat Tracking Challenge       //
//                                                                   //
//   Please read the associated documentation on the procedure of    //
//   building the source code into executable app package and also   //
//   the installation.                                               //
//===================================================================//

#pragma once

#include <assert.h>
#include <string>
#include <memory.h>

//==================================================//
// Helper Class CLock
// Would help to automatically acquire/release a lock.
//==================================================//
class CLock
{
public:
    bool m_locked;
    CRITICAL_SECTION* m_pCS;

    void Lock() { if (m_locked) return; EnterCriticalSection(m_pCS); m_locked = true; }
    void Unlock() { if (!m_locked) return; LeaveCriticalSection(m_pCS); m_locked = false; }

    CLock(CRITICAL_SECTION* pCS) : m_pCS(pCS), m_locked(false) { Lock(); }
    CLock(const CLock& other) = delete;
    ~CLock() { Unlock(); }
};

class CLockSRW
{
public:
    bool m_locked;
    PSRWLOCK m_plock;

    void Lock() { if (m_locked) return; AcquireSRWLockExclusive(m_plock); m_locked = true; }
    void Unlock() { if (!m_locked) return; ReleaseSRWLockExclusive(m_plock); m_locked = false; }

    CLockSRW(SRWLOCK& lock) : m_plock(&lock), m_locked(false) { Lock(); }
    CLockSRW(const CLock& other) = delete;
    ~CLockSRW() { Unlock(); }
};

//==================================================//
// Audio Sample Callback
//==================================================//
class CSampleCallback
{
public:
    virtual void OnSampleReady(void* Provider) = 0;
    virtual void OnSampleFinish(void* Provider) = 0;
};

//==================================================//
// Reference Countable Class Base
//==================================================//
class CReference
{
protected:
    bool m_bRefEnabled;
    int m_nRef;

    //!@! the following line is critical.
    virtual ~CReference() {}

    virtual void EnableReferenceInternal(bool bEnable)
    {
        bool bLast = m_bRefEnabled;
        m_bRefEnabled = bEnable;
        if (m_bRefEnabled != bLast)
        {
            if (m_bRefEnabled)
                //Enabled
                m_nRef = 1;
            else
                //Disabled
                m_nRef = 1;
        }
    }

public:
    virtual int ReferenceCount() { return (m_bRefEnabled ? m_nRef : 1); }
    virtual int AddRef() {
        if (m_bRefEnabled) {
            return (int)InterlockedIncrement((ULONG*)&m_nRef);
        }
        else {
            return 1;
        }
    }
    virtual int Release() {
        if (m_bRefEnabled)
        {
            int Current = (int)InterlockedDecrement((ULONG*)&m_nRef);
            if (Current == 0)
                delete this;
            return Current;
        }
        else return 1;
    }

    CReference()
    {
        m_nRef = 1; m_bRefEnabled = true;
    }
};

//==================================================//
//Some memory helper macros/inlines
//==================================================//
template<class TYPE> inline void Safe_Free(TYPE*& p) { if (p) { free(p); (p) = NULL; } }
template<class TYPE> inline void Safe_Delete(TYPE*& p) { if (p) { delete (p); (p) = NULL; } }
template<class TYPE> inline void Safe_Delete_Array(TYPE*& p) { if (p) { delete[](p); (p) = NULL; } }
template<class TYPE> inline void Safe_Release(TYPE*& p) { if (p) { p->Release(); (p) = NULL; } }
template<class TYPE> inline void Safe_Set(TYPE*& p, TYPE* v) { Safe_Release(p); if (v) { p = v; v->AddRef(); } }

#define SAFE_DELETE(p) { if (p) { delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p)=NULL; } }


//================
#define block_to_complete(x) (create_task(x).wait())

