# final
Нужно создать проект на github и написать веб-сервер. Протокол HTTP 1.0, нужно реализовать только команду GET
(POST - опционально), ответы 200 и 404, а также MIME-тип text/html (другие типы, например image/jpeg - опционально).

Веб-сервер должен запускаться командой:
/home/box/final/final -h <ip> -p <port> -d <directory>

После запуска сервер обязан вернуть управление, то есть должен демонизироваться.
Сервер должен быть или многопоточным или многопроцессным с передачей дескрипторов.
