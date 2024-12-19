﻿#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#include "ComponentBase.h"
#include "AddInDefBase.h"
#include "IMemoryManager.h"
#include <string>

///////////////////////////////////////////////////////////////////////////////
// class CAddInNative
class CAddInNative : public IComponentBase
{
public:
    enum Props
    {
        eAdressProp,   // Адрес
        ePortProp,     // Порт
        eLastProp      // Always last
    };

    enum Methods
    {
        eStartServerMethod, // Старт сервера
        eGetMessageMethod,  // Получаем первое сообщения из пула сообщений
        eGetStatusMethod,   // Получаем статус сервера
        eStopServerMethod,  // Остановка сервера
        eLastMethod         // Always last
    };

    CAddInNative(void);
    virtual ~CAddInNative();
    // IInitDoneBase
    virtual bool ADDIN_API Init(void*) override;
    virtual bool ADDIN_API setMemManager(void* mem) override;
    virtual long ADDIN_API GetInfo() override;
    virtual void ADDIN_API Done() override;
    // ILanguageExtenderBase
    virtual bool ADDIN_API RegisterExtensionAs(WCHAR_T**) override;
    virtual long ADDIN_API GetNProps() override;
    virtual long ADDIN_API FindProp(const WCHAR_T* wsPropName) override;
    virtual const WCHAR_T* ADDIN_API GetPropName(long lPropNum, long lPropAlias) override;
    virtual bool ADDIN_API GetPropVal(const long lPropNum, tVariant* pvarPropVal) override;
    virtual bool ADDIN_API SetPropVal(const long lPropNum, tVariant* varPropVal) override;
    virtual bool ADDIN_API IsPropReadable(const long lPropNum) override;
    virtual bool ADDIN_API IsPropWritable(const long lPropNum) override;
    virtual long ADDIN_API GetNMethods() override;
    virtual long ADDIN_API FindMethod(const WCHAR_T* wsMethodName) override;
    virtual const WCHAR_T* ADDIN_API GetMethodName(const long lMethodNum,
        const long lMethodAlias) override;
    virtual long ADDIN_API GetNParams(const long lMethodNum) override;
    virtual bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum,
        tVariant* pvarParamDefValue) override;
    virtual bool ADDIN_API HasRetVal(const long lMethodNum) override;
    virtual bool ADDIN_API CallAsProc(const long lMethodNum,
        tVariant* paParams, const long lSizeArray) override;
    virtual bool ADDIN_API CallAsFunc(const long lMethodNum,
        tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray) override;
    // LocaleBase
    virtual void ADDIN_API SetLocale(const WCHAR_T* loc) override;
    // UserLanguageBase
    virtual void ADDIN_API SetUserInterfaceLanguageCode(const WCHAR_T* lang) override;

private:
    long findName(const wchar_t* names[], const wchar_t* name, const uint32_t size) const;
    void addError(uint32_t wcode, const wchar_t* source,
        const wchar_t* descriptor, long code);
    void addError(uint32_t wcode, const char16_t* source,
        const char16_t* descriptor, long code);

    void startUdpServer() const; // Старт сервера
    void stopUdpServer();        // Остановка сервера
    bool isRunning();  // Метод для проверки состояния работы сервера
    void setPvarRetValue(const std::string& messageVal, tVariant* pvarRetValue); // Убираем дублирование кода
    std::string getFirstMessageFromServer(); // Получение первого сообщения из пула сообщений
 
    // Attributes
    IAddInDefBase* m_iConnect;
    IMemoryManager* m_iMemory;

    wchar_t* m_pAdressProp = 0;  // Адрес
    uint32_t m_pPortProp = 0;   // Порт
};
#endif //__ADDINNATIVE_H__
