# BirdsFeeder_IoT
# Функционал:

**Проверка уровня корма.**
Каждые 20 минут или при клике на круговую диаграмму в правом верхнем углу фронт посылает запрос на проверку уровня корма и отображает его в правом верхнем углу сайта, указывая дату и время последней проверки и уровень корма в процентах (считаем, что кормушка заполнена, если расстояние от корма до датчика меньше 2см и что размер кормушки = sizeBox (10)).

**Скачивание фотографии.**
Для скачивания последней сделанной фотографии необходимо кликнуть на вторую слева иконку. Откроется новое окно с фотографией.

**Фото.**
Чтобы сделать фото необходимо нажать на соответствующую иконку в левом верхнем углу. Кнопка фото и скачивания заморозятся на 5 секунд. (необходимо для корректной записи и изъятия фотографии)

**Счетчик птиц и автофотография.**
В loop кормушка проверяет присутствие птицы. Если появилась новая птица, то счетчик птиц увеличивается (и записывается в соответствующий файл внутренней памяти устройства) и камера делает снимок. При этом, каждые 5 секунд фронт выполняет функцию birdCheck(), которая делает соответствующий запрос и в случае необходимости обновляет фотографию.
