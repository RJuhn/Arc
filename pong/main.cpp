//linker::system::subsystem  - Windows(/ SUBSYSTEM:WINDOWS)
//configuration::advanced::character set - not set
//linker::input::additional dependensies Msimg32.lib; Winmm.lib

#include "windows.h"
#include "cmath"

// ������ ������ ����  
typedef struct {
    float x, y, width, height, rad, dx, dy, speed;
    bool status;
    HBITMAP hBitmap;//����� � ������� ������ 
} sprite;


//test 1
//
//
// end


const int Xblocks = 16;
const int Yblocks = 5;

sprite racket;//������� ������
sprite blocks[Xblocks][Yblocks];
sprite ball;//�����

struct {
    int score, balls;//���������� ��������� ����� � ���������� "������"
    bool action = false;//��������� - �������� (����� ������ ������ ������) ��� ����
} game;

struct {
    HWND hWnd;//����� ����
    HDC device_context, context;// ��� ��������� ���������� (��� �����������)
    int width, height;//���� �������� ������� ���� ������� ������� ���������
} window;

HBITMAP hBack;// ����� ��� �������� �����������

//c����� ����

void InitGame()
{
    //� ���� ������ ��������� ������� � ������� ������� gdi
    //���� ������������� - ����� ������ ������ ����� � .exe 
    //��������� ������ LoadImageA ��������� � ������� ��������, ��������� �������� ����� ������������� � ������� ���� �������
    ball.hBitmap = (HBITMAP)LoadImageA(NULL, "ball.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    racket.hBitmap = (HBITMAP)LoadImageA(NULL, "racket.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    hBack = (HBITMAP)LoadImageA(NULL, "back.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    //------------------------------------------------------

    racket.width = 300;
    racket.height = 50;
    racket.speed = 50;//�������� ����������� �������
    racket.x = window.width / 2.;//������� ���������� ����
    racket.y = window.height - racket.height;//���� ���� ���� ������ - �� ������ �������


    ball.dy = -(rand() % 65 + 35) / 100.;//��������� ������ ������ ������
    ball.dx = -(1 - ball.dy);//��������� ������ ������ ������
    ball.speed = 10;
    ball.rad = 20;
    ball.x = racket.x;//x ���������� ������ - �� ������� �������
    ball.y = racket.y - ball.rad;//����� ����� ������ �������

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
//���������� �����

bool DebugModSwitch;

void DebugMod() {

    POINT cursorPos;

    if (GetCursorPos(&cursorPos));
    ScreenToClient(window.hWnd, &cursorPos);
    //ShowCursor !!
    //����� ��������

    SetTextColor(window.context, RGB(255, 7, 0));
    SetBkColor(window.context, RGB(0, 0, 0));
    SetBkMode(window.context, TRANSPARENT);
    auto hFont = CreateFont(50, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "CALIBRI");
    auto hTmp = (HFONT)SelectObject(window.context, hFont);

    char txt1[32];//����� ��� ������
    _itoa_s(cursorPos.x, txt1, 10);//�������������� �������� ���������� � �����. ����� �������� � ���������� txt
    TextOutA(window.context, 1450, 10, "CursorPoint.x", 13);
    TextOutA(window.context, 1700, 10, (LPCSTR)txt1, strlen(txt1));

    _itoa_s(cursorPos.y, txt1, 10);
    TextOutA(window.context, 1450, 100, "CursorPoint.y", 13);
    TextOutA(window.context, 1700, 100, (LPCSTR)txt1, strlen(txt1));
    ball.x = cursorPos.x;
    ball.y = cursorPos.y;

};

void ProcessSound(const char* name)//������������ ���������� � ������� .wav, ���� ������ ������ � ��� �� ����� ��� � ���������
{
    //PlaySound(TEXT(name), NULL, SND_FILENAME | SND_ASYNC);//���������� name �������� ��� �����. ���� ASYNC ��������� ����������� ���� ���������� � ����������� ���������
}

void ShowScore()
{
    //�������� �������� � �������
    SetTextColor(window.context, RGB(160, 160, 160));
    SetBkColor(window.context, RGB(0, 0, 0));
    SetBkMode(window.context, TRANSPARENT);
    auto hFont = CreateFont(70, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "CALIBRI");
    auto hTmp = (HFONT)SelectObject(window.context, hFont);

    char txt[32];//����� ��� ������
    _itoa_s(game.score, txt, 10);//�������������� �������� ���������� � �����. ����� �������� � ���������� txt
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

    hMemDC = CreateCompatibleDC(hDC); // ������� �������� ������, ����������� � ���������� �����������
    hOldbm = (HBITMAP)SelectObject(hMemDC, hBitmapBall);// �������� ����������� bitmap � �������� ������

    if (hOldbm) // ���� �� ���� ������, ���������� ������
    {
        GetObject(hBitmapBall, sizeof(BITMAP), (LPSTR)&bm); // ���������� ������� �����������

        if (alpha)
        {
            TransparentBlt(window.context, x, y, x1, y1, hMemDC, 0, 0, x1, y1, RGB(0, 0, 0));//��� ������� ������� ����� ����� ��������������� ��� ����������
        }
        else
        {
            StretchBlt(hDC, x, y, x1, y1, hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY); // ������ ����������� bitmap
        }

        SelectObject(hMemDC, hOldbm);// ��������������� �������� ������
    }

    DeleteDC(hMemDC); // ������� �������� ������
}

void ShowRacketAndBall()
{
    ShowBitmap(window.context, 0, 0, window.width, window.height, hBack);//������ ���
    ShowBitmap(window.context, racket.x - racket.width / 2., racket.y, racket.width, racket.height, racket.hBitmap);// ������� ������


    for (int X = 0; X < Xblocks; X++) {

        for (int Y = 0; Y < Yblocks; Y++) {

            if (blocks[X][Y].status == true) {

                ShowBitmap(window.context, blocks[X][Y].x, blocks[X][Y].y, blocks[X][Y].width, blocks[X][Y].height, blocks[X][Y].hBitmap);
            }

        }
    }

    //  ShowBitmap(window.context, ball.x - ball.rad, ball.y - ball.rad, 2 * ball.rad, 2 * ball.rad, ball.hBitmap, true);// �����
}

void LimitRacket()
{
    racket.x = max(racket.x, racket.width / 2.);//���� ��������� ������ ���� ������� ������ ����, �������� �� ����
    racket.x = min(racket.x, window.width - racket.width / 2.);//���������� ��� ������� ����
}

void ProcessRoom()
{
    bool tail = false;
    //������������ �����, ������� � ���. ������� - ���� ������� ����� ���� ���������, � ������, ��� ������� �� ����� ������ ������������� ����� ������� �������� ������
    //CheckFloor
    if (ball.y > window.height - ball.rad - racket.height)
    {
        if (ball.x >= racket.x - racket.width / 2. - ball.rad &&
            ball.x <= racket.x + racket.width / 2. + ball.rad)
        {
            game.score++;
            ball.speed += 5. / game.score;
            ball.dy = -ball.dy;
            ball.y = racket.y - ball.rad;
            racket.width -= 10. / game.score;
            ProcessSound("bounce.wav");

            ball.y = racket.y - ball.rad;
        }
        else if (ball.y - ball.rad > window.height)
        {
            // ������ ������
            game.balls--;
            ProcessSound("fail.wav");

            if (game.balls <= 0) {
                MessageBoxA(window.hWnd, "game over", "", MB_OK);
                InitGame();
                return;
            }

            // ����� ������
            ball.dy = -(rand() % 65 + 35) / 100.;
            ball.dx = (rand() % 100 - 50) / 100.;
            ball.x = racket.x;
            ball.y = racket.y - ball.rad;
            game.action = false;
            tail = false;
        }
    }

    //CheckBlocks
    else {
        float nextX = ball.dx * ball.speed;
        float nextY = ball.dy * ball.speed;
        float L = sqrt(nextX * nextX + nextY * nextY);
        
        float StepX = nextX / L;
        float StepY = nextY / L;
        float StartX = ball.x;
        float StartY = ball.y;

        float stepS = 2;
        bool Collision = false;

        for (float i = 0; i < L; i += stepS) {
            StartX += StepX * stepS;
            StartY += StepY * stepS;

            SetPixel(window.context, StartX, StartY, RGB(255, 255, 255));

            // CheckWalls
            if (StartX < ball.rad) {
                StepX = -StepX;
                StartX = ball.rad;
                Collision = true;
                ProcessSound("bounce.wav");
            }
            else if (StartX > window.width - ball.rad) {
                StepX = -StepX; // ����������� �������� �����
                StartX = window.width - ball.rad;
                Collision = true;
                ProcessSound("bounce.wav");
            }
            //CheckRoof
            else if (StartY < ball.rad) {
                StepY = -StepY; // ����������� �������� ����
                StartY = ball.rad;
                Collision = true;
                ProcessSound("bounce.wav");
            }
            //CheckBlock
            else {
                for (int X = 0; X < Xblocks; X++) {
                    for (int Y = 0; Y < Yblocks; Y++) {

                        if (blocks[X][Y].status) {
                            float block_x1 = blocks[X][Y].x;
                            float block_x2 = blocks[X][Y].x + blocks[X][Y].width;
                            float block_y1 = blocks[X][Y].y;
                            float block_y2 = blocks[X][Y].y + blocks[X][Y].height;

                            if ((StartX >= block_x1 && StartX <= block_x2) && 
                                (StartY >= block_y1 && StartY <= block_y2)) {

                                float top = (StartY - block_y1);
                                float bottom = (block_y2 - StartY);
                                float left = (StartX - block_x1);
                                float right = (block_x2 - StartX);

                                float xmin = min(left, right);
                                float ymin = min(top, bottom);

                                if (xmin < ymin) {
                                    StepX = -StepX; // �������������� ������
                                }
                                else {
                                    StepY = -StepY; // ������������ ������
                                }

                                if (!GetAsyncKeyState(VK_F3)) {
                                    blocks[X][Y].status = false;
                                }

                                Collision = true;
                                ProcessSound("bounce.wav");
                            }
                        }
                    }
                }
            }

            if (Collision) {
                break;
            }
        }

        if (game.action) {
            ball.x = StartX;
            ball.y = StartY;
        
            
            if (Collision) {
                // ������������� ball.dx � ball.dy �� StepX, StepY
                float newLength = sqrt(StepX * StepX + StepY * StepY);
                ball.dx = StepX / newLength;
                ball.dy = StepY / newLength;
            }
        }
    }
}

void ProcessBall()
{
    if (game.action)
    {
        //���� ���� � �������� ������ - ���������� �����
    //    ball.x += ball.dx * ball.speed;
     //   ball.y += ball.dy * ball.speed;
    }
    else
    {
        //����� - ����� "��������" � �������
        ball.x = racket.x;
    }
}

void InitWindow()
{
    SetProcessDPIAware();
    window.hWnd = CreateWindow("edit", 0, WS_POPUP | WS_VISIBLE | WS_MAXIMIZE, 0, 0, 0, 0, 0, 0, 0, 0);

    RECT r;
    GetClientRect(window.hWnd, &r);
    window.device_context = GetDC(window.hWnd);//�� ������ ���� ������� ����� ��������� ���������� ��� ���������
    window.width = r.right - r.left;//���������� ������� � ���������
    window.height = r.bottom - r.top;
    window.context = CreateCompatibleDC(window.device_context);//������ �����
    SelectObject(window.context, CreateCompatibleBitmap(window.device_context, window.width, window.height));//����������� ���� � ���������
    GetClientRect(window.hWnd, &r);

}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow) {

    InitWindow();//����� �������������� ��� ��� ����� ��� ��������� � ����
    InitGame();//����� �������������� ���������� ����

    //mciSendString(TEXT("play ..\\Debug\\music.mp3 repeat"), NULL, 0, NULL);
    ShowCursor(NULL);

    while (!GetAsyncKeyState(VK_ESCAPE))
    {
        ShowRacketAndBall();//������ ���, ������� � �����
        ShowScore();//������ ���� � �����

        ProcessInput();//����� ����������
        LimitRacket();//���������, ����� ������� �� ������� �� �����

        ProcessRoom();//������������ ������� �� ���� � �������, ��������� ������ � ��������

        BitBlt(window.device_context, 0, 0, window.width, window.height, window.context, 0, 0, SRCCOPY);//�������� ����� � ����
        Sleep(16);//���� 16 ���������� (1/���������� ������ � �������)

        ProcessBall();//���������� �����

    }

}