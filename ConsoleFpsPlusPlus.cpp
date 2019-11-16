// ConsoleFpsPlusPlus.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>
using namespace std;

int nScreenWidth = 120;
int nScreenHeight = 40;

float fPlayerX = 5.0f;
float fPlayerY = 10.0f;
float fPlayerAngle = -0.5f;

int nMapHeight = 16;
int nMapWidth = 16;

float fFov = 3.14159 / 4.0;
float fDepth = 16.0f;

int main()
{
	wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	wstring map;

	map += L"################";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..........#...#";
	map += L"#..........#...#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#........#######";
	map += L"#..............#";
	map += L"#..............#";
	map += L"################";

	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();

	while (1)
	{
		tp2 = chrono::system_clock::now();
		chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		// Controls
		// Handle player rotation
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
		{
			fPlayerAngle -= (3.0f) * fElapsedTime;
		}

		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
		{
			fPlayerAngle += (3.0f) * fElapsedTime;
		}

		if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
		{
			fPlayerX += sinf(fPlayerAngle) * 5.0f * fElapsedTime;
			fPlayerY += cosf(fPlayerAngle) * 5.0f * fElapsedTime;

			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX -= sinf(fPlayerAngle) * 5.0f * fElapsedTime;
				fPlayerY -= cosf(fPlayerAngle) * 5.0f * fElapsedTime;
			}
		}

		if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
		{
			fPlayerX -= sinf(fPlayerAngle) * 5.0f * fElapsedTime;
			fPlayerY -= cosf(fPlayerAngle) * 5.0f * fElapsedTime;

			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX += sinf(fPlayerAngle) * 5.0f * fElapsedTime;
				fPlayerY += cosf(fPlayerAngle) * 5.0f * fElapsedTime;
			}
		}

		for (int x = 0; x < nScreenWidth; x++)
		{
			// For each column, calculate the projected ray angle into the world space
			float fRayAngle = (fPlayerAngle - fFov / 2.0f) + ((float)x / (float)nScreenWidth) * fFov;

			float fDistanceToWall = 0.0f;

			bool bHitWall = false;
			bool bBoundary = false;

			float fEyeX = sinf(fRayAngle); // Unit vector for ray in player space
			float fEyeY = cosf(fRayAngle);

			while (!bHitWall && fDistanceToWall < fDepth)
			{
				fDistanceToWall += 0.1f;

				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
				{
					bHitWall = true; // Just set distance to maximum depth
					fDistanceToWall = fDepth;
				}
				else
				{
					// Ray is inbounds so test to see if the ray cell is a wall block
					// check here is causing rendering issue - is rendering flipped xy values e.g. 5,10 will render at 10,5
					// changed from video code (map[nTestX * nMapWidth + nTestY]) to the below (map[nTestY * nMapWidth + nTestX])
					// I am unsure why this was causing values to be flipped. This works in the video.
					if (map[nTestY * nMapWidth + nTestX] == '#')
					{
						bHitWall = true;

						vector<pair<float, float>> p; // distance, dot

						for (int tx = 0; tx < 2; tx++)
						{
							for (int ty = 0; ty < 2; ty++)
							{
								float vy = (float)nTestY + ty - fPlayerY;
								float vx = (float)nTestX + tx - fPlayerX;
								float d = sqrt(vx*vx + vy*vy);
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(make_pair(d, dot));
							}
						}

						// Sort Pairs from closest to farthest
						sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) { return left.first < right.first; });

						float fBound = 0.01;
						if (acos(p.at(0).second) < fBound) { bBoundary = true; }
						if (acos(p.at(1).second) < fBound) { bBoundary = true; }
						if (acos(p.at(2).second) < fBound) { bBoundary = true; }
					}
				}
			}

			// Calculate distance to ceiling and floor
			int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			short nShade = ' ';

			if (fDistanceToWall <= fDepth / 4.0f) { nShade = 0x2588;}
			else if (fDistanceToWall <= fDepth / 3.0f) { nShade = 0x2593;}
			else if (fDistanceToWall <= fDepth / 2.0f) { nShade = 0x2592; }
			else if (fDistanceToWall <= fDepth) { nShade = 0x2591; }
			else { nShade = ' '; }

			if (bBoundary) { nShade = ' ';  } // Black it out

			for (int y = 0; y < nScreenHeight; y++)
			{
				if (y < nCeiling)
				{
					screen[y * nScreenWidth + x] = ' ';
				}
				else if(y > nCeiling && y <= nFloor)
				{
					screen[y * nScreenWidth + x] = nShade;
				}
				else
				{
					// Shade floor based on distance
					float fFloorDistance = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
					short nShadeFloor = ' ';
					if (fFloorDistance < 0.25) { nShadeFloor = '#'; }
					else if (fFloorDistance < 0.5) { nShadeFloor = 'x'; }
					else if (fFloorDistance < 0.75) { nShadeFloor = '.'; }
					else if (fFloorDistance < 0.9) { nShadeFloor = '-'; }

					screen[y * nScreenWidth + x] = nShadeFloor;
				}
			}
		}

		// Display Stats
		swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f, FPS=%3.2f", fPlayerX, fPlayerY, fPlayerAngle, 1.0f / fElapsedTime);

		// Display Map
		for (int nx = 0; nx < nMapWidth; nx++)
		{
			for (int ny = 0; ny < nMapWidth; ny++)
			{
				screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
			}
		}

		screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = 'P';

		screen[nScreenWidth * nScreenHeight - 1] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}

	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
