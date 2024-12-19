#include <boost/asio.hpp>
#include <boost/locale.hpp>
#include "stdafx.h"

#define CONVERT_ERROR -1

#if defined(__linux__) || defined(__APPLE__)
#include <uchardet/uchardet.h>
#include <errno.h>
#include <iconv.h>
#include <signal.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <wchar.h>

#include <codecvt>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "AddInNative.h"
#include "UdpServer.h"

static const wchar_t* g_PropNames[] = {L"eAdressProp", L"ePortProp"};
static const wchar_t* g_MethodNames[] = {
    L"eStartSertverMethod", L"eGetMessageMethod", L"eGetStatusMethod",
    L"eStopServerMethod"};

static const wchar_t* g_PropNamesRu[] = {L"Адрес", L"Порт"};
static const wchar_t* g_MethodNamesRu[] = {L"СерверСтарт", L"ПолучитьСообщение",
                                           L"ПолучитьСтатусСервера",
                                           L"ОстановкаСервера"};

static const WCHAR_T g_kClassNames[] =
    u"CAddInNative";  //|OtherClass1|OtherClass2";
static IAddInDefBase* pAsyncEvent = NULL;

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source,
                          size_t len = 0);   
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source,
                            uint32_t len = 0);
uint32_t getLenShortWcharStr(const WCHAR_T* Source);

std::unique_ptr<UdpServer> server = nullptr;
static bool is_server_running = false;
std::mutex server_running_mutex;
std::atomic<bool> server_running(false);

static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;
static std::u16string s_names(g_kClassNames);
//---------------------------------------------------------------------------//
long GetClassObject(const WCHAR_T* wsName, IComponentBase** pInterface) {
  if (!*pInterface) {
    *pInterface = new CAddInNative();
    return (long)*pInterface;
  }
  return 0;
}
//---------------------------------------------------------------------------//
AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities) {
  g_capabilities = capabilities;
  return eAppCapabilitiesLast;
}
//---------------------------------------------------------------------------//
AttachType GetAttachType() { return eCanAttachAny; }
//---------------------------------------------------------------------------//
long DestroyObject(IComponentBase** pIntf) {
  if (!*pIntf) return -1;

  delete *pIntf;
  *pIntf = 0;
  return 0;
}
//---------------------------------------------------------------------------//
const WCHAR_T* GetClassNames() { return s_names.c_str(); }
//---------------------------------------------------------------------------//
// CAddInNative
CAddInNative::CAddInNative() {}
//---------------------------------------------------------------------------//
CAddInNative::~CAddInNative() {}
//---------------------------------------------------------------------------//
bool CAddInNative::Init(void* pConnection) {
  m_iConnect = (IAddInDefBase*)pConnection;
  return m_iConnect != NULL;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetInfo() { return 2000; }
//---------------------------------------------------------------------------//
void CAddInNative::Done() {}
/////////////////////////////////////////////////////////////////////////////
// ILanguageExtenderBase
//---------------------------------------------------------------------------//
bool CAddInNative::RegisterExtensionAs(WCHAR_T** wsExtensionName) {
  wchar_t* wsExtension = L"AddInNativeUdpServer";
  int iActualSize = ::wcslen(wsExtension) + 1;

  if (m_iMemory) {
    if (m_iMemory->AllocMemory((void**)wsExtensionName,
                               iActualSize * sizeof(WCHAR_T)))
      ::convToShortWchar(wsExtensionName, wsExtension, iActualSize);
    return true;
  }

  return false;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNProps() { return eLastProp; }
//---------------------------------------------------------------------------//
long CAddInNative::FindProp(const WCHAR_T* wsPropName) {
  long plPropNum = -1;
  wchar_t* propName = 0;

  ::convFromShortWchar(&propName, wsPropName);
  plPropNum = findName(g_PropNames, propName, eLastProp);

  if (plPropNum == -1) plPropNum = findName(g_PropNamesRu, propName, eLastProp);

  delete[] propName;

  return plPropNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetPropName(long lPropNum, long lPropAlias) {
  if (lPropNum >= eLastProp) return NULL;

  wchar_t* wsCurrentName = NULL;
  WCHAR_T* wsPropName = NULL;
  size_t iActualSize = 0;

  switch (lPropAlias) {
    case 0:  // First language
      wsCurrentName = (wchar_t*)g_PropNames[lPropNum];
      break;
    case 1:  // Second language
      wsCurrentName = (wchar_t*)g_PropNamesRu[lPropNum];
      break;
    default:
      return 0;
  }

  iActualSize = wcslen(wsCurrentName) + 1;

  if (m_iMemory && wsCurrentName) {
    if (m_iMemory->AllocMemory((void**)&wsPropName,
                               (unsigned)iActualSize * sizeof(WCHAR_T)))
      ::convToShortWchar(&wsPropName, wsCurrentName, iActualSize);
  }

  return wsPropName;
}
//---------------------------------------------------------------------------//
bool CAddInNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal) {
  switch (lPropNum) {
    case eAdressProp: {
      if (m_iMemory) {
        TV_VT(pvarPropVal) = VTYPE_PWSTR;
        auto iActualSize = ::wcslen(m_pAdressProp);

        if (m_iMemory->AllocMemory((void**)&(pvarPropVal->pwstrVal),
                                   iActualSize * sizeof(WCHAR_T))) {
          ::convToShortWchar(&(pvarPropVal->pwstrVal), m_pAdressProp,
                             iActualSize);
          pvarPropVal->wstrLen = iActualSize;
        }
      } else
        TV_VT(pvarPropVal) = VTYPE_EMPTY;
    } break;
    case ePortProp: {
      if (m_iMemory) {
        TV_VT(pvarPropVal) = VTYPE_UI4;
        TV_UI4(pvarPropVal) = m_pPortProp;
      } else
        TV_VT(pvarPropVal) = VTYPE_EMPTY;
    } break;
    default:
      return false;
  }

  return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::SetPropVal(const long lPropNum, tVariant* varPropVal) {
  switch (lPropNum) {
    case eAdressProp:
      ::convFromShortWchar(&m_pAdressProp, TV_WSTR(varPropVal));
      break;
    case ePortProp: {
      if (TV_VT(varPropVal) != VTYPE_I4) return false;
      m_pPortProp = TV_UI4(varPropVal);
      break;
    }
    default:
      return false;
  }

  return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropReadable(const long lPropNum) {
  switch (lPropNum) {
    case eAdressProp:
    case ePortProp:
      return true;
    default:
      return false;
  }

  return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropWritable(const long lPropNum) {
  switch (lPropNum) {
    case eAdressProp:
    case ePortProp:
      return true;
    default:
      return false;
  }

  return false;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNMethods() { return eLastMethod; }
//---------------------------------------------------------------------------//
long CAddInNative::FindMethod(const WCHAR_T* wsMethodName) {
  long plMethodNum = -1;
  wchar_t* name = 0;

  ::convFromShortWchar(&name, wsMethodName);

  plMethodNum = findName(g_MethodNames, name, eLastMethod);

  if (plMethodNum == -1)
    plMethodNum = findName(g_MethodNamesRu, name, eLastMethod);

  delete[] name;

  return plMethodNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetMethodName(const long lMethodNum,
                                           const long lMethodAlias) {
  if (lMethodNum >= eLastMethod) return NULL;

  wchar_t* wsCurrentName = NULL;
  WCHAR_T* wsMethodName = NULL;
  size_t iActualSize = 0;

  switch (lMethodAlias) {
    case 0:  // First language
      wsCurrentName = (wchar_t*)g_MethodNames[lMethodNum];
      break;
    case 1:  // Second language
      wsCurrentName = (wchar_t*)g_MethodNamesRu[lMethodNum];
      break;
    default:
      return 0;
  }

  iActualSize = wcslen(wsCurrentName) + 1;

  if (m_iMemory && wsCurrentName) {
    if (m_iMemory->AllocMemory((void**)&wsMethodName,
                               (unsigned)iActualSize * sizeof(WCHAR_T)))
      ::convToShortWchar(&wsMethodName, wsCurrentName, iActualSize);
  }

  return wsMethodName;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNParams(const long lMethodNum) {
  switch (lMethodNum) {
    case eStartServerMethod:
      return 0;
    case eGetMessageMethod:
      return 0;
    case eGetStatusMethod:
      return 0;
    case eStopServerMethod:
      return 0;
    default:
      return 0;
  }

  return 0;
}
//---------------------------------------------------------------------------//
bool CAddInNative::GetParamDefValue(const long lMethodNum, const long lParamNum,
                                    tVariant* pvarParamDefValue) {
  return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::HasRetVal(const long lMethodNum) {
  switch (lMethodNum) {
    case eStartServerMethod:
      return true;
    case eGetMessageMethod:
      return true;
    case eGetStatusMethod:
      return true;
    case eStopServerMethod:
      return true;
    default:
      return false;
  }

  return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::CallAsProc(const long lMethodNum, tVariant* paParams,
                              const long lSizeArray) {
  return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue,
                              tVariant* paParams, const long lSizeArray) {
                    
  bool is_running = false;
  switch (lMethodNum) {
    case eStartServerMethod:
      if (pvarRetValue) {
          // Запускаем сервер
          startUdpServer();
          // Даем серверу время на запуск
          std::this_thread::sleep_for(std::chrono::seconds(1));
          is_running = isRunning();
          is_server_running = is_running;
          setPvarRetValue(is_running ? u8"Работает" : u8"Не работает",
                          pvarRetValue);
      }
      return true;
    case eGetMessageMethod:
      if (pvarRetValue) {
        if (server != nullptr){
           auto message = getFirstMessageFromServer();
          setPvarRetValue(message, pvarRetValue);
        } else {
          setPvarRetValue(u8"non", pvarRetValue);
        }
      }
      return true;
    case eGetStatusMethod:
      if (pvarRetValue) {
        is_running = isRunning();
        setPvarRetValue(is_running ? u8"Работает" : u8"Не работает",
                        pvarRetValue);
      }
      return true;
    case eStopServerMethod:
      if (pvarRetValue) {
        // Остановка сервера.
        stopUdpServer();
        is_running = isRunning();
        setPvarRetValue(is_running ? u8"Работает" : u8"Не работает",
                        pvarRetValue);
      }
      return true;
    default:
      return false;
  }

  return false;
}
//---------------------------------------------------------------------------//
void CAddInNative::SetLocale(const WCHAR_T* loc) {}
//---------------------------------------------------------------------------//
void ADDIN_API CAddInNative::SetUserInterfaceLanguageCode(const WCHAR_T* lang) {
}
//---------------------------------------------------------------------------//
bool CAddInNative::setMemManager(void* mem) {
  m_iMemory = (IMemoryManager*)mem;
  return m_iMemory != 0;
}

void CAddInNative::addError(uint32_t wcode, const wchar_t* source,
                            const wchar_t* descriptor, long code) {
  if (m_iConnect) {
    WCHAR_T* err = 0;
    WCHAR_T* descr = 0;

    ::convToShortWchar(&err, source);
    ::convToShortWchar(&descr, descriptor);

    m_iConnect->AddError(wcode, err, descr, code);
    delete[] err;
    delete[] descr;
  }
}
//---------------------------------------------------------------------------//
void CAddInNative::addError(uint32_t wcode, const char16_t* source,
                            const char16_t* descriptor, long code) {
  if (m_iConnect) {
    m_iConnect->AddError(wcode, source, descriptor, code);
  }
}
//---------------------------------------------------------------------------//
// Старт сревера.
void CAddInNative::startUdpServer() const {
  std::lock_guard<std::mutex> lock(server_running_mutex); // Защита доступа к серверу

  std::string adress(m_pAdressProp, m_pAdressProp + wcslen(m_pAdressProp));
  if (server != nullptr) {
    server->stop();
  }

  bool success = false;
  int attempts = 0;
  while (!success && attempts < 5) {
    try {
      server = std::make_unique<UdpServer>(adress, m_pPortProp);
      success = true;
    }
    catch (const std::exception& e) {
      attempts++;
    }
  }

  if (!success) {
    return;
  }

  server_running = true;

  std::mutex mtx;
  std::condition_variable cv;

  std::future<void> server_future = std::async(std::launch::async, [&]() {
    server->run();
    {
      std::lock_guard<std::mutex> lock(mtx);
      server_running = false;
    }
    cv.notify_all();
  });

  {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&]() {
      return !server_running;
    });
  }
}
//---------------------------------------------------------------------------//
// Метод для завершения работы сервера
void CAddInNative::stopUdpServer() {
  if (server != nullptr) {
    server->stop();
    server_running = false;
    server = nullptr;
  }
}
//---------------------------------------------------------------------------//
bool CAddInNative::isRunning() {
  if (server != nullptr) {
    bool isRunning = (bool)server->isRunning();
    return isRunning;
  }
  return false;
}

void CAddInNative::setPvarRetValue(const std::string& messageVal,
                                   tVariant* pvarRetValue) {                                                                                                                      
  // Преобразование из UTF-8 в wide string (wstring)
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  std::wstring ws_allMessage = converter.from_bytes(messageVal);
  const wchar_t* wchar_ptr = ws_allMessage.c_str();
  size_t iActualSize = ::wcslen(wchar_ptr);

  TV_VT(pvarRetValue) = VTYPE_PWSTR;
  if (m_iMemory) {
    if (m_iMemory->AllocMemory((void**)&(pvarRetValue->pwstrVal),
                               iActualSize * sizeof(WCHAR_T))) {
      ::convToShortWchar(&(pvarRetValue->pwstrVal), wchar_ptr, iActualSize);
      pvarRetValue->wstrLen = iActualSize;
    }
  }
}
//---------------------------------------------------------------------------//
// Получение сообщения.
std::string CAddInNative::getFirstMessageFromServer() {
  if (server) {
    return server->getFirstMessage();
  }
  return u8"non";
}
//---------------------------------------------------------------------------//
long CAddInNative::findName(const wchar_t* names[], const wchar_t* name,
                            const uint32_t size) const {
  long ret = -1;
  for (uint32_t i = 0; i < size; i++) {
    if (!wcscmp(names[i], name)) {
      ret = i;
      break;
    }
  }
  return ret;
}
//---------------------------------------------------------------------------//
uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, size_t len) {
  if (!len) len = ::wcslen(Source) + 1;
  if (!*Dest) *Dest = new WCHAR_T[len];

  WCHAR_T* tmpShort = *Dest;
  wchar_t* tmpWChar = (wchar_t*)Source;
  uint32_t res = 0;

  ::memset(*Dest, 0, len * sizeof(WCHAR_T));

#if defined(__linux__) || defined(__APPLE__)
  size_t succeed = (size_t)-1;
  size_t f = len * sizeof(wchar_t), t = len * sizeof(WCHAR_T);
  const char* fromCode = sizeof(wchar_t) == 2 ? "UTF-16" : "UTF-32";
  iconv_t cd = iconv_open("UTF-16LE", fromCode);
  if (cd != (iconv_t)-1) {
    succeed = iconv(cd, (char**)&tmpWChar, &f, (char**)&tmpShort, &t);
    iconv_close(cd);
    if (succeed != (size_t)-1) return (uint32_t)succeed;
  }
#endif
  for (; len; --len, ++res, ++tmpWChar, ++tmpShort) {
    *tmpShort = (WCHAR_T)*tmpWChar;
  }
  return res;
}
//---------------------------------------------------------------------------//
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source,
                            uint32_t len) {

  if (!len) len = getLenShortWcharStr(Source) + 1;

  if (!*Dest) *Dest = new wchar_t[len];

  wchar_t* tmpWChar = *Dest;
  WCHAR_T* tmpShort = (WCHAR_T*)Source;
  uint32_t res = 0;

  ::memset(*Dest, 0, len * sizeof(WCHAR_T));

#if defined(__linux__) || defined(__APPLE__)
  size_t succeed = (size_t)-1;
  const char* fromCode = sizeof(wchar_t) == 2 ? "UTF-16" : "UTF-32";
  size_t f = len * sizeof(WCHAR_T), t = len * sizeof(wchar_t);
  iconv_t cd = iconv_open("UTF-32LE", fromCode);
  if (cd != (iconv_t)-1) {
    succeed = iconv(cd, (char**)&tmpShort, &f, (char**)&tmpWChar, &t);
    iconv_close(cd);
    if (succeed != (size_t)-1) return (uint32_t)succeed;
  }
#endif
  for (; len; --len, ++res, ++tmpWChar, ++tmpShort) {
    *tmpWChar = (wchar_t)*tmpShort;
  }

  return res;
}
//---------------------------------------------------------------------------//
uint32_t getLenShortWcharStr(const WCHAR_T* Source) {
  uint32_t res = 0;
  WCHAR_T* tmpShort = (WCHAR_T*)Source;

  while (*tmpShort++) ++res;

  return res;
}
//---------------------------------------------------------------------------//



