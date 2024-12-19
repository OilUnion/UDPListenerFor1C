Не то чтобы рабочий проект. Скорее для теста, чем для реальных задач.
Проблемы: 
- при отправке пакета больше 40_000 байт аварийно завершается. Ошибки не возникает при отправке пакета размера 31_500 байт;
- при отправке русских символов аварийно завершается. Ошибка была если отправляли из Windows. Решили костылем, а именно перед отправкой закодировали в UTF8. Отправку из Linux прошел нормально.

```1C
// Подключение компоненты.
УстановитьВнешнююКомпоненту("ОбщийМакет.AddInNative");
ПодключитьВнешнююКомпоненту("ОбщийМакет.AddInNative", "CAddInNative", ТипВнешнейКомпоненты.Native);
МояКомпонента = Новый("AddIn.CAddInNative.AddInNativeUdpServer");

// Старт сервера.
МояКомпонента.Адрес = "127.0.0.1";
МояКомпонента.Порт = 12345; 
МояКомпонента.СерверСтарт();

// МояКомпонента.ПолучитьСтатусСервера() - получает статус сервера работает/не работает
Сообщить(МояКомпонента.ПолучитьСтатусСервера());

// МояКомпонента.ПолучитьСообщение() - получает первое из списка сообщений пришедших по udp.
// Если сообщений нет то приходит "non"
Сообщить(МояКомпонента.ПолучитьСообщение()); 

// Остановка сервера.
МояКомпонента.ОстановкаСервера(); 
```

МояКомпонента.ПолучитьСообщение(). Так как реализация примитивная, то сообщения приходят в очередь.
Данная функция достает один элемент, а потом удаляет из очереди. Чтобы получить все сообщения, можно использовать `Цикл`.



