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


    ball.dy = -(rand() % 65 + 35) / 100.;//формируем вектор полета шарика
    ball.dx = -(1 - ball.dy);//формируем вектор полета шарика
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
//Отладочный режим

bool DebugModSwitch;

void DebugMod() {

    POINT cursorPos;

    if (GetCursorPos(&cursorPos));
    ScreenToClient(window.hWnd, &cursorPos);
    //ShowCursor !!
    //вывод значений

    SetTextColor(window.context, RGB(255, 7, 0));
    SetBkColor(window.context, RGB(0, 0, 0));
    SetBkMode(window.context, TRANSPARENT);
    auto hFont = CreateFont(50, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "CALIBRI");
    auto hTmp = (HFONT)SelectObject(window.context, hFont);

    char txt1[32];//буфер для текста
    _itoa_s(cursorPos.x, txt1, 10);//преобразование числовой переменной в текст. текст окажется в переменной txt
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

bool tail = false;

bool CheckWalls(float checkX, float checkY) {
    if (checkX < 0 || checkX > window.width) {
        return true;
    }
    return false;
}

bool CheckRoof(float checkX, float checkY) {
    if (checkY < 0) {
        return true;
    }
    return false;
}
int CheckBlocks(float checkX, float checkY) {
    for (int X = 0; X < Xblocks; X++) {
        for (int Y = 0; Y < Yblocks; Y++) {
            if (!blocks[X][Y].status) continue;

            float block_x1 = blocks[X][Y].x;
            float block_x2 = blocks[X][Y].x + blocks[X][Y].width;
            float block_y1 = blocks[X][Y].y;
            float block_y2 = blocks[X][Y].y + blocks[X][Y].height;

            if (checkX > block_x1 and checkX < block_x2 and
                checkY > block_y1 and checkY < block_y2) {

                float left = checkX - block_x1;
                float right = block_x2 - checkX;
                float top = checkY - block_y1;
                float bottom = block_y2 - checkY;

                float xmin = min(left, right);
                float ymin = min(top, bottom);

                if (!GetAsyncKeyState(VK_F3)) {
                    blocks[X][Y].status = false;
                }

                if (xmin < ymin) return 1;
                else return -1;
            }
        }
    }
    return 0;
}

int CheckFloor(float checkX, float checkY) {
    // Для пола проверяем по ЦЕНТРУ шарика (ракетка)
    if (checkY > window.height - ball.rad - racket.height) {
        if (!tail and checkX >= racket.x - racket.width / 2 - ball.rad and
            checkX <= racket.x + racket.width / 2 + ball.rad) {
            // Отбитие
            game.score++;
            //ball.speed += 5 / game.score;
            racket.width -= 10 / game.score;
            ProcessSound("bounce.wav");
            return 1;
        }
        else {
            tail = true;
            if (checkY > window.height) {
                game.balls--;
                ProcessSound("fail.wav");
                if (game.balls <= 0) {
                    MessageBoxA(window.hWnd, "game over", "", MB_OK);
                    InitGame();
                }
                return 2;
            }
        }
    }
    return 0;
}

void Physics() {
    if (!game.action) return;

    float timeLeft = 1.0;
    float currentX = ball.x;
    float currentY = ball.y;
    float currentDx = ball.dx;
    float currentDy = ball.dy;

    while (timeLeft > 0.001 and traceCount < 20) {
        float nextX = currentX + currentDx * ball.speed * timeLeft;
        float nextY = currentY + currentDy * ball.speed * timeLeft;

        bool collision = false;

        float angle = atan2(currentDy, currentDx);
        float startAngle = angle - 3.14159 / 2;  // -90°
        float endAngle = angle + 3.14159 / 2;    // +90°

        for (float a = startAngle; a <= endAngle and !collision; a += (3.14159 / 6)) {
            // Точка на полуокружности
            float checkX = nextX + cos(a) * ball.rad;
            float checkY = nextY + sin(a) * ball.rad;

            if (CheckWalls(checkX, checkY)) {
                currentDx = -currentDx;
                timeLeft *= 0.5;
                collision = true;
                break;
            }

            if (CheckRoof(checkY, checkY)) {
                currentDy = -currentDy;
                timeLeft *= 0.5;
                collision = true;
                break;
            }

            int checkblock = CheckBlocks(checkX, checkY);
            if (checkblock == 1) {
                currentDx = -currentDx;
                timeLeft *= 0.5;
                collision = true;
                break;
            }
            else if (checkblock == -1) {
                currentDy = -currentDy;
                timeLeft *= 0.5;
                collision = true;
                break;
            }

            int checkfloor = CheckFloor(nextX, nextY);
            if (checkfloor == 1) {
                currentDy = - currentDy;
                timeLeft *= 0.5;
                collision = true;
                break;
            }
            else if (checkfloor == 2) {
                currentDy = -(rand() % 65 + 35) / 100;
                currentDx = -(1 - currentDy);
                currentX = racket.x;
                currentY = racket.y - ball.rad;
                game.action = false;
                tail = false;
                collision = true;
                break;
            }
        }

        if (!collision) {
            currentX = nextX;
            currentY = nextY;
            break;
        }
    }

    ball.x = currentX;
    ball.y = currentY;
    ball.dx = currentDx;
    ball.dy = currentDy;
}

void DraweTraceline(float x1, float y1, float x2, float y2) {
    float steps = 100.0;
    for (int i = 0; i <= steps; i++) {
        float t = i / steps;
        float x = x1 + (x2 - x1) * t;
        float y = y1 + (y2 - y1) * t;
        SetPixel(window.context, x, y, RGB(255, 255, 255));
    }
}


void DrawAtanLine() {
    if (!game.action) return;

    float traceX = ball.x;
    float traceY = ball.y;
    float traceDx = ball.dx;
    float traceDy = ball.dy;

    for (int i = 0; i < 5; i++) {
        float nextX = traceX + traceDx * ball.speed * 1.0;
        float nextY = traceY + traceDy * ball.speed * 1.0;

        // ОТРИСОВКА ПОЛУОКРУЖНОСТИ в начальной точке
        float currentAngle = atan2(traceDy, traceDx);
        float startAngle = currentAngle - 3.14159 / 2;
        float endAngle = currentAngle + 3.14159 / 2;

        for (float a = startAngle; a <= endAngle; a += 0.1) {
            float x = traceX + cos(a) * ball.rad;
            float y = traceY + sin(a) * ball.rad;
            SetPixel(window.context, x, y, RGB(255, 255, 255));
        }

        bool collision = false;

        for (float a = startAngle; a <= endAngle and !collision; a += (3.14159 / 6)) {
            float checkX = nextX + cos(a) * ball.rad;
            float checkY = nextY + sin(a) * ball.rad;

            SetPixel(window.context, checkX, checkY, RGB(255, 255, 255));

            if (checkX < 0 || checkX > window.width) {
                traceDx = -traceDx;
                collision = true;
                break;
            }

            if (checkY < 0) {
                traceDy = -traceDy;
                collision = true;
                break;
            }

            for (int X = 0; X < Xblocks and !collision; X++) {
                for (int Y = 0; Y < Yblocks and !collision; Y++) {
                    if (!blocks[X][Y].status) continue;

                    float block_x1 = blocks[X][Y].x;
                    float block_x2 = blocks[X][Y].x + blocks[X][Y].width;
                    float block_y1 = blocks[X][Y].y;
                    float block_y2 = blocks[X][Y].y + blocks[X][Y].height;

                    if (checkX > block_x1 && checkX < block_x2 &&
                        checkY > block_y1 && checkY < block_y2) {

                        float left = checkX - block_x1;
                        float right = block_x2 - checkX;
                        float top = checkY - block_y1;
                        float bottom = block_y2 - checkY;

                        float xmin = min(left, right);
                        float ymin = min(top, bottom);

                        if (xmin < ymin) {
                            traceDx = -traceDx;
                        }
                        else {
                            traceDy = -traceDy;
                        }
                        collision = true;
                        break;
                    }
                }
            }

            if (checkY > window.height - ball.rad - racket.height) {
                if (nextX >= racket.x - racket.width / 2 - ball.rad &&
                    nextX <= racket.x + racket.width / 2 + ball.rad) {
                    traceDy = -traceDy;
                    collision = true;
                    break;
                }
                else {
                    DraweTraceline(traceX, traceY, nextX, nextY);
                    return;
                }
            }
        }

        DraweTraceline(traceX, traceY, nextX, nextY);

        if (!collision) {
            traceX = nextX;
            traceY = nextY;
        }
    }
}

void ProcessRoom()
{
    //обрабатываем стены, потолок и пол. принцип - угол падения равен углу отражения, а значит, для отскока мы можем просто инвертировать часть вектора движения шарика
    Physics();
    DrawAtanLine();
}

void ProcessBall()
{
    if (game.action)
    {
        //если игра в активном режиме - перемещаем шарик
        ball.x += ball.dx * ball.speed;
        ball.y += ball.dy * ball.speed;
    }
    else
    {
        //иначе - шарик "приклеен" к ракетке
        ball.x = racket.x;
    }
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
