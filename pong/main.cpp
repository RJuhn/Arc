//linker::system::subsystem  - Windows(/ SUBSYSTEM:WINDOWS)
//configuration::advanced::character set - not set
//linker::input::additional dependensies Msimg32.lib; Winmm.lib

#include "windows.h"
#include "cmath"

// секция данных игры  
typedef struct {
    float x, y, width, height, rad, dx, dy, speed;
    bool status;
    HBITMAP hBitmap;//хэндл к спрайту шарика 
} sprite;

typedef struct {
    float x, y, dx, dy, time;
} TraceSegment;

TraceSegment traceHistory;
int traceCount = 0;

const int Xblocks = 16;
const int Yblocks = 5;

sprite racket;//ракетка игрока
sprite blocks[Xblocks][Yblocks];
sprite ball;//шарик

struct {
    int score, balls;//количество набранных очков и оставшихся "жизней"
    bool action = false;//состояние - ожидание (игрок должен нажать пробел) или игра
} game;

struct {
    HWND hWnd;//хэндл окна
    HDC device_context, context;// два контекста устройства (для буферизации)
    int width, height;//сюда сохраним размеры окна которое создаст программа
} window;

HBITMAP hBack;// хэндл для фонового изображения

struct RayResult {
    bool hit;
    float hitX, hitY;
    float normalX, normalY;
    float distance;
    int hitType;
};

enum HitType {
    WALL_X = 0,
    WALL_Y = 1,
    BLOCK = 2,
    RACKET = 3,
    FLOOR = 4
};


//cекция кода

void InitGame()
{
    //в этой секции загружаем спрайты с помощью функций gdi
    //пути относительные - файлы должны лежать рядом с .exe 
    //результат работы LoadImageA сохраняет в хэндлах битмапов, рисование спрайтов будет произовдиться с помощью этих хэндлов
    ball.hBitmap = (HBITMAP)LoadImageA(NULL, "ball.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    racket.hBitmap = (HBITMAP)LoadImageA(NULL, "racket.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    hBack = (HBITMAP)LoadImageA(NULL, "back.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    //------------------------------------------------------

    racket.width = 300;
    racket.height = 50;
    racket.speed = 50;//скорость перемещения ракетки
    racket.x = window.width / 2.;//ракетка посередине окна
    racket.y = window.height - racket.height;//чуть выше низа экрана - на высоту ракетки


    float angle = (rand() % 120 + 30) * 3.14159 / 180.0;
    ball.dx = cos(angle);
    ball.dy = -sin(angle);
    ball.speed = 10;
    ball.rad = 20;
    ball.x = racket.x;//x координата шарика - на середие ракетки
    ball.y = racket.y - ball.rad;//шарик лежит сверху ракетки

    game.score = 0;
    game.balls = 9;

    for (int X = 0; X < Xblocks; X++) {

        for (int Y = 0; Y < Yblocks; Y++) {

            blocks[X][Y].width = window.width / Xblocks;
            blocks[X][Y].height = window.height / Yblocks / 3;

            blocks[X][Y].x = blocks[X][Y].width * X;
            blocks[X][Y].y = blocks[X][Y].height * Y + window.height / 3;

            blocks[X][Y].hBitmap = racket.hBitmap;

            blocks[X][Y].status = true;
        }
    }

}


void Normalize(float& dx, float& dy) {
    float len = sqrt(dx * dx + dy * dy);
    if (len > 0) {
        dx /= len;
        dy /= len;
    }
}

void Reflect(float& dx, float& dy, float nx, float ny) {
    float dot = dx * nx + dy * ny;
    dx = dx - 2 * dot * nx;
    dy = dy - 2 * dot * ny;
}

// === ЯДРО ФИЗИКИ - ОДНА ФУНКЦИЯ ДЛЯ ВСЕХ ПРОВЕРОК ===
RayResult CheckCollision(float startX, float startY, float dirX, float dirY, float maxDist, bool forTrace = false) {
    RayResult result;
    result.hit = false;
    result.distance = maxDist;

    if (dirX != 0) {
        // Левая стена
        float t = (ball.rad - startX) / dirX;
        if (t > 0 && t < result.distance) {
            float y = startY + dirY * t;
            if (y >= ball.rad && y <= window.height - ball.rad) {
                result.hit = true;
                result.distance = t;
                result.hitX = ball.rad;
                result.hitY = y;
                result.normalX = 1.0;
                result.normalY = 0.0;
                result.hitType = WALL_X;
            }
        }

        // Правая стена
        t = (window.width - ball.rad - startX) / dirX;
        if (t > 0 && t < result.distance) {
            float y = startY + dirY * t;
            if (y >= ball.rad && y <= window.height - ball.rad) {
                result.hit = true;
                result.distance = t;
                result.hitX = window.width - ball.rad;
                result.hitY = y;
                result.normalX = -1.0;
                result.normalY = 0.0;
                result.hitType = WALL_X;
            }
        }
    }
    // Потолок
    if (dirY != 0) {
        float t = (ball.rad - startY) / dirY;
        if (t > 0 && t < result.distance) {
            float x = startX + dirX * t;
            if (x >= ball.rad && x <= window.width - ball.rad) {
                result.hit = true;
                result.distance = t;
                result.hitX = x;
                result.hitY = ball.rad;
                result.normalX = 0.0;
                result.normalY = 1.0;
                result.hitType = WALL_Y;
            }
        }
    }

    // БЛОКИ
    for (int X = 0; X < Xblocks; X++) {
        for (int Y = 0; Y < Yblocks; Y++) {
            if (!blocks[X][Y].status) continue;

            // Расширенный блок
            float left = blocks[X][Y].x - ball.rad;
            float right = blocks[X][Y].x + blocks[X][Y].width + ball.rad;
            float top = blocks[X][Y].y - ball.rad;
            float bottom = blocks[X][Y].y + blocks[X][Y].height + ball.rad;

            // Алгоритм пересечения луча с прямоугольником
            float t1 = (left - startX) / dirX;
            float t2 = (right - startX) / dirX;
            float t3 = (top - startY) / dirY;
            float t4 = (bottom - startY) / dirY;

            float tmin = max(min(t1, t2), min(t3, t4));
            float tmax = min(max(t1, t2), max(t3, t4));

            if (tmin < tmax&& tmax > 0 && tmin < result.distance) {
                float t = tmin > 0 ? tmin : tmax;
                if (t > 0) {
                    result.hit = true;
                    result.distance = t;
                    result.hitX = startX + dirX * t;
                    result.hitY = startY + dirY * t;
                    result.hitType = BLOCK;

                    // Определяем нормаль
                    float cx = blocks[X][Y].x + blocks[X][Y].width / 2;
                    float cy = blocks[X][Y].y + blocks[X][Y].height / 2;
                    float dx = result.hitX - cx;
                    float dy = result.hitY - cy;

                    if (fabs(dx) > fabs(dy)) {
                        result.normalX = (dx > 0) ? -1.0 : 1.0;
                        result.normalY = 0.0;
                    }
                    else {
                        result.normalX = 0.0f;
                        result.normalY = (dy > 0) ? -1.0 : 1.0;
                    }
                }
            }
        }
    }

    // 4. РАКЕТКА (только если движемся вниз)
    if (dirY > 0) {
        float racketTop = racket.y - ball.rad;
        float t = (racketTop - startY) / dirY;

        if (t > 0 && t < result.distance) {
            float x = startX + dirX * t;
            float racketLeft = racket.x - racket.width / 2 - ball.rad;
            float racketRight = racket.x + racket.width / 2 + ball.rad;

            if (x >= racketLeft && x <= racketRight) {
                result.hit = true;
                result.distance = t;
                result.hitX = x;
                result.hitY = racketTop;
                result.normalX = 0.0;
                result.normalY = -1.0;
                result.hitType = RACKET;
            }
        }
    }

    // ПОЛ
    if (dirY > 0) {
        float t = (window.height - ball.rad - startY) / dirY;
        if (t > 0 && t < result.distance) {
            result.hit = true;
            result.distance = t;
            result.hitX = startX + dirX * t;
            result.hitY = window.height - ball.rad;
            result.normalX = 0.0;
            result.normalY = -1.0;
            result.hitType = FLOOR;
        }
    }

    return result;
}

// === ФИЗИКА ИГРЫ ===
void Physics() {
    if (!game.action) return;

    float timeLeft = 1.0;
    float posX = ball.x + ball.rad; // Центр шарика
    float posY = ball.y + ball.rad;
    float dirX = ball.dx;
    float dirY = ball.dy;

    Normalize(dirX, dirY);

    while (timeLeft > 0.001) {
        // Максимальное расстояние за оставшееся время
        float maxDist = ball.speed * timeLeft;

        // Проверяем столкновения
        RayResult hit = CheckCollision(posX, posY, dirX, dirY, maxDist, false);

        if (hit.hit) {
            // Двигаем до точки столкновения
            posX += dirX * hit.distance;
            posY += dirY * hit.distance;

            // Обрабатываем столкновение
            switch (hit.hitType) {
            case WALL_X:
            case WALL_Y:
                Reflect(dirX, dirY, hit.normalX, hit.normalY);
                break;

            case BLOCK: {
                // Находим и уничтожаем блок
                for (int X = 0; X < Xblocks; X++) {
                    for (int Y = 0; Y < Yblocks; Y++) {
                        if (!blocks[X][Y].status) continue;

                        if (posX >= blocks[X][Y].x - ball.rad &&
                            posX <= blocks[X][Y].x + blocks[X][Y].width + ball.rad &&
                            posY >= blocks[X][Y].y - ball.rad &&
                            posY <= blocks[X][Y].y + blocks[X][Y].height + ball.rad) {
                            blocks[X][Y].status = false;
                            game.score += 10;
                            break;
                        }
                    }
                }
                Reflect(dirX, dirY, hit.normalX, hit.normalY);
                break;
            }

            case RACKET: {
                Reflect(dirX, dirY, hit.normalX, hit.normalY);
                // Добавляем немного горизонтали в зависимости от места удара
                float hitPos = (posX - racket.x) / (racket.width / 2);
                dirX += hitPos * 0.3;
                Normalize(dirX, dirY);
                game.score += 5;
                break;
            }

            case FLOOR: {
                // Потеря шарика
                game.balls--;
                game.action = false;
                ball.x = racket.x;
                ball.y = racket.y - ball.rad;

                if (game.balls <= 0) {
                    MessageBoxA(window.hWnd, "Game Over", "Арканоид", MB_OK);
                    // Перезапуск игры
                    game.score = 0;
                    game.balls = 9;
                    for (int X = 0; X < Xblocks; X++)
                        for (int Y = 0; Y < Yblocks; Y++)
                            blocks[X][Y].status = true;
                }
                return;
            }
            }

            // Уменьшаем оставшееся время
            timeLeft -= hit.distance / ball.speed;

            // Сдвигаем чуть-чуть от точки столкновения
            posX += dirX * 0.00001;
            posY += dirY * 0.00001;
        }
        else {
            // Нет столкновений - двигаем на полное расстояние
            posX += dirX * maxDist;
            posY += dirY * maxDist;
            break;
        }
    }

    // Обновляем позицию шарика
    ball.x = posX - ball.rad;
    ball.y = posY - ball.rad;
    ball.dx = dirX;
    ball.dy = dirY;
}

// трассировка
void DrawTrace() {
    if (!game.action) return;

    // центр шарика
    float startX = ball.x + ball.rad;
    float startY = ball.y + ball.rad;
    float dirX = ball.dx;
    float dirY = ball.dy;

    Normalize(dirX, dirY);

    // Ограничиваем трассировку 3 отскоками или 300 пикселями
    int maxBounces = 3;
    float maxTotalDistance = 300.0;
    float traveled = 0.0;
    int bounces = 0;

    while (bounces < maxBounces && traveled < maxTotalDistance) {
        // Доступное расстояние для этого отрезка
        float rayLength = maxTotalDistance - traveled;

        // Ищем столкновение
        RayResult hit = CheckCollision(startX, startY, dirX, dirY, rayLength, true);

        float segmentLength;
        if (hit.hit) {
            segmentLength = hit.distance;
        }
        else {
            segmentLength = rayLength;
        }

        // Рисуем отрезок траектории
        // Разбиваем на 10 частей, рисуем полуокружности
        int parts = 10;
        for (int i = 0; i <= parts; i++) {
            float t = (float)i / parts;
            float cx = startX + dirX * segmentLength * t;
            float cy = startY + dirY * segmentLength * t;

            // Угол направления
            float angle = atan2(dirY, dirX);

            // Рисуем полуокружность (180° впереди)
            for (float a = angle - 1.57f; a <= angle + 1.57; a += 0.1) {
                float px = cx + cos(a) * ball.rad;
                float py = cy + sin(a) * ball.rad;

                // Проверяем границы экрана
                if (px >= 0 && px <= window.width && py >= 0 && py <= window.height) {
                    SetPixel(window.context, (int)px, (int)py, RGB(255, 100, 100));
                }
            }

            // Соединяем точки (кроме первой итерации)
            if (i > 0) {
                float prevX = startX + dirX * segmentLength * ((float)(i - 1) / parts);
                float prevY = startY + dirY * segmentLength * ((float)(i - 1) / parts);

                // Рисуем линию между центрами
                float steps = 20.0f;
                for (int s = 0; s <= steps; s++) {
                    float st = (float)s / steps;
                    float lx = prevX + (cx - prevX) * st;
                    float ly = prevY + (cy - prevY) * st;

                    if (lx >= 0 && lx <= window.width && ly >= 0 && ly <= window.height) {
                        SetPixel(window.context, (int)lx, (int)ly, RGB(255, 255, 255));
                    }
                }
            }
        }

        traveled += segmentLength;

        if (hit.hit && hit.hitType != FLOOR) {
            // Отражаем луч для следующего отрезка
            Reflect(dirX, dirY, hit.normalX, hit.normalY);
            startX = hit.hitX;
            startY = hit.hitY;
            bounces++;
        }
        else {
            break;
        }
    }
}


bool DebugModSwitch;

void DebugMod() {

    POINT cursorPos;

    if (GetCursorPos(&cursorPos));
    ScreenToClient(window.hWnd, &cursorPos);

    SetTextColor(window.context, RGB(255, 7, 0));
    SetBkColor(window.context, RGB(0, 0, 0));
    SetBkMode(window.context, TRANSPARENT);
    auto hFont = CreateFont(50, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "CALIBRI");
    auto hTmp = (HFONT)SelectObject(window.context, hFont);

    char txt1[32];
    _itoa_s(cursorPos.x, txt1, 10);
    TextOutA(window.context, 1450, 10, "CursorPoint.x", 13);
    TextOutA(window.context, 1700, 10, (LPCSTR)txt1, strlen(txt1));

    _itoa_s(cursorPos.y, txt1, 10);
    TextOutA(window.context, 1450, 100, "CursorPoint.y", 13);
    TextOutA(window.context, 1700, 100, (LPCSTR)txt1, strlen(txt1));

    //float angle = atan2(ball.dy, ball.dx);
    //_itoa_s(angle, txt1, 14);
    //TextOutA(window.context, 1450, 150, "Atan2", 13);
    //TextOutA(window.context, 1700, 150, (LPCSTR)txt1, strlen(txt1));

    ball.x = cursorPos.x;
    ball.y = cursorPos.y;

};

void ProcessSound(const char* name)//проигрывание аудиофайла в формате .wav, файл должен лежать в той же папке где и программа
{
    //PlaySound(TEXT(name), NULL, SND_FILENAME | SND_ASYNC);//переменная name содежрит имя файла. флаг ASYNC позволяет проигрывать звук паралельно с исполнением программы
}

void ShowScore()
{
    //поиграем шрифтами и цветами
    SetTextColor(window.context, RGB(160, 160, 160));
    SetBkColor(window.context, RGB(0, 0, 0));
    SetBkMode(window.context, TRANSPARENT);
    auto hFont = CreateFont(70, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "CALIBRI");
    auto hTmp = (HFONT)SelectObject(window.context, hFont);

    char txt[32];//буфер для текста
    _itoa_s(game.score, txt, 10);//преобразование числовой переменной в текст. текст окажется в переменной txt
    TextOutA(window.context, 10, 10, "Score", 5);
    TextOutA(window.context, 200, 10, (LPCSTR)txt, strlen(txt));

    _itoa_s(game.balls, txt, 10);
    TextOutA(window.context, 10, 100, "Balls", 5);
    TextOutA(window.context, 200, 100, (LPCSTR)txt, strlen(txt));

}

void ProcessInput()
{
    if (GetAsyncKeyState(VK_F3)) { DebugMod(); }
    if (GetAsyncKeyState(VK_LEFT)) racket.x -= racket.speed;
    if (GetAsyncKeyState(VK_RIGHT)) racket.x += racket.speed;

    if (!game.action && GetAsyncKeyState(VK_SPACE))
    {
        game.action = true;
        ProcessSound("bounce.wav");
    }
}

void ShowBitmap(HDC hDC, int x, int y, int x1, int y1, HBITMAP hBitmapBall, bool alpha = false)
{
    HBITMAP hbm, hOldbm;
    HDC hMemDC;
    BITMAP bm;

    hMemDC = CreateCompatibleDC(hDC); // Создаем контекст памяти, совместимый с контекстом отображения
    hOldbm = (HBITMAP)SelectObject(hMemDC, hBitmapBall);// Выбираем изображение bitmap в контекст памяти

    if (hOldbm) // Если не было ошибок, продолжаем работу
    {
        GetObject(hBitmapBall, sizeof(BITMAP), (LPSTR)&bm); // Определяем размеры изображения

        if (alpha)
        {
            TransparentBlt(window.context, x, y, x1, y1, hMemDC, 0, 0, x1, y1, RGB(0, 0, 0));//все пиксели черного цвета будут интепретированы как прозрачные
        }
        else
        {
            StretchBlt(hDC, x, y, x1, y1, hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY); // Рисуем изображение bitmap
        }

        SelectObject(hMemDC, hOldbm);// Восстанавливаем контекст памяти
    }

    DeleteDC(hMemDC); // Удаляем контекст памяти
}

void ShowRacketAndBall()
{
    ShowBitmap(window.context, 0, 0, window.width, window.height, hBack);//задний фон
    ShowBitmap(window.context, racket.x - racket.width / 2., racket.y, racket.width, racket.height, racket.hBitmap);// ракетка игрока


    for (int X = 0; X < Xblocks; X++) {

        for (int Y = 0; Y < Yblocks; Y++) {

            if (blocks[X][Y].status == true) {

                ShowBitmap(window.context, blocks[X][Y].x, blocks[X][Y].y, blocks[X][Y].width, blocks[X][Y].height, blocks[X][Y].hBitmap);
            }

        }
    }

     // ShowBitmap(window.context, ball.x - ball.rad, ball.y - ball.rad, 2 * ball.rad, 2 * ball.rad, ball.hBitmap, true);// шарик
}

void LimitRacket()
{
    racket.x = max(racket.x, racket.width / 2.);//если коодината левого угла ракетки меньше нуля, присвоим ей ноль
    racket.x = min(racket.x, window.width - racket.width / 2.);//аналогично для правого угла
}



void ProcessRoom()
{
    //обрабатываем стены, потолок и пол. принцип - угол падения равен углу отражения, а значит, для отскока мы можем просто инвертировать часть вектора движения шарика
    Physics();      // вместо старой Physics
    DrawTrace();
}



void InitWindow()
{
    SetProcessDPIAware();
    window.hWnd = CreateWindow("edit", 0, WS_POPUP | WS_VISIBLE | WS_MAXIMIZE, 0, 0, 0, 0, 0, 0, 0, 0);

    RECT r;
    GetClientRect(window.hWnd, &r);
    window.device_context = GetDC(window.hWnd);//из хэндла окна достаем хэндл контекста устройства для рисования
    window.width = r.right - r.left;//определяем размеры и сохраняем
    window.height = r.bottom - r.top;
    window.context = CreateCompatibleDC(window.device_context);//второй буфер
    SelectObject(window.context, CreateCompatibleBitmap(window.device_context, window.width, window.height));//привязываем окно к контексту
    GetClientRect(window.hWnd, &r);

}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow) {

    InitWindow();//здесь инициализируем все что нужно для рисования в окне
    InitGame();//здесь инициализируем переменные игры

    //mciSendString(TEXT("play ..\\Debug\\music.mp3 repeat"), NULL, 0, NULL);
    ShowCursor(NULL);

    while (!GetAsyncKeyState(VK_ESCAPE))
    {
        ShowRacketAndBall();//рисуем фон, ракетку и шарик
        ShowScore();//рисуем очик и жизни
        //DrawTrace();

        ProcessInput();//опрос клавиатуры
        LimitRacket();//проверяем, чтобы ракетка не убежала за экран

        ProcessRoom();//обрабатываем отскоки от стен и каретки, попадание шарика в картетку

        BitBlt(window.device_context, 0, 0, window.width, window.height, window.context, 0, 0, SRCCOPY);//копируем буфер в окно
        Sleep(16);//ждем 16 милисекунд (1/количество кадров в секунду)

        //ProcessBall();//перемещаем шарик

    }

}
