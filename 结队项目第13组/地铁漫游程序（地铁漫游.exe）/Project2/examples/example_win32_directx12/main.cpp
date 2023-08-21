// Dear ImGui: standalone example application for DirectX 12
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// Important: to compile on 32-bit systems, the DirectX12 backend requires code to be compiled with '#define ImTextureID ImU64'.
// This is because we need ImTextureID to carry a 64-bit value and by default ImTextureID is defined as void*.
// This define is set in the example .vcxproj file and need to be replicated in your app or by adding it to your imconfig.h file.

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <ctime>
#include <math.h>
#include <Windows.h>
#include <codecvt>
#include <locale>

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64                  FenceValue;
};

// Data
static int const                    NUM_FRAMES_IN_FLIGHT = 3;
static FrameContext                 g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
static UINT                         g_frameIndex = 0;

static int const                    NUM_BACK_BUFFERS = 3;
static ID3D12Device*                g_pd3dDevice = nullptr;
static ID3D12DescriptorHeap*        g_pd3dRtvDescHeap = nullptr;
static ID3D12DescriptorHeap*        g_pd3dSrvDescHeap = nullptr;
static ID3D12CommandQueue*          g_pd3dCommandQueue = nullptr;
static ID3D12GraphicsCommandList*   g_pd3dCommandList = nullptr;
static ID3D12Fence*                 g_fence = nullptr;
static HANDLE                       g_fenceEvent = nullptr;
static UINT64                       g_fenceLastSignaledValue = 0;
static IDXGISwapChain3*             g_pSwapChain = nullptr;
static HANDLE                       g_hSwapChainWaitableObject = nullptr;
static ID3D12Resource*              g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForLastSubmittedFrame();
FrameContext* WaitForNextFrameResources();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ----------------------------------------------------------------------------
using namespace std;

const int INF = 0x7f7f7f7f;
const int MAX_POINT = 500; //车站的个数上限
const int MAX_LINE = 50; //路线的个数上限


string line_name[MAX_LINE]; // 路线的名字
vector<int> line[MAX_LINE]; // 路线中有哪些车站
map<string, int> point_index;	//每个站的标号
string point_name[MAX_POINT];	// 每个站的名字
vector<int> neighbor[MAX_POINT];	// 点a的第i个邻居的站点序号为：neighbor[a][i]				//
vector<int> cost[MAX_POINT];        // 点a到第i个邻居的距离为：cost[a][i]						// 邻居的顺序是由输入文件决定的
vector<int> from_line[MAX_POINT];   // 点a到第i个邻居的地铁路线序号为：from_line[a][i]			//
int dis[MAX_POINT];		//dis[a]表示起点到a点的最短距离
bool vis[MAX_POINT];	//标记
int pre[MAX_POINT];     //起点到a点的最短路径上，a点之前的一个点是pre[a] 
//因此到a点的最短路径为 start_point -> ... -> pre[pre[a]] -> pre[a] -> a
int pre_line[MAX_POINT];//a到pre[a]的路线属于pre_line路线
int belong_line[MAX_POINT];//记录每个点属于哪条线
bool is_change_point[MAX_POINT];// 标记是否是换乘站
int line_start_time[MAX_LINE], line_end_time[MAX_LINE];
int now_time;

bool vis_func3[MAX_POINT];	//功能3专用标记
int remaining_point = -1;	//功能三剩余点数
bool consider_transfer_cost = 0;//考虑换乘开销

int line_count = 0;// 线路数

int point_count = 0;// 车站数

int fun = 0;// 标识是哪个fun调用此模块


vector<int> v_func3_vi;// display_func3_vi 的专用容器，每次调用完后记得释放内存空间！！！
int remaining_steps = 0;// 记录剩余步数，记得reset！！！

int city_map_flag = 0;//0北京，1武汉

// -------------------------------------------------------------------------------------------------

// Helper functions

// Display prompt then read zero-terminated, UTF-8 password.
// Return password length with terminator, or zero on error.
static int
read_password(char *buf, int len, char *prompt) {
    /* Resources that will be cleaned up */
    int pwlen = 0;
    DWORD orig = 0;
    WCHAR *wbuf = 0;
    SIZE_T wbuf_len = 0;
    HANDLE hi, ho = INVALID_HANDLE_VALUE;

    /* Set up input console handle */
    DWORD access = GENERIC_READ | GENERIC_WRITE;
    hi = CreateFileA("CONIN$", access, 0, 0, OPEN_EXISTING, 0, 0);
    if (!GetConsoleMode(hi, &orig)) goto done;
    DWORD mode = orig;
    mode |= ENABLE_PROCESSED_INPUT;
    mode |= ENABLE_LINE_INPUT;
    //mode &= ~ENABLE_ECHO_INPUT;
    if (!SetConsoleMode(hi, mode)) goto done;

    /* Set up output console handle */
    ho = CreateFileA("CONOUT$", GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (!WriteConsoleA(ho, prompt, (DWORD)strlen(prompt), 0, 0)) goto done;

    /* Allocate a wide character buffer the size of the output */
    wbuf_len = (len - 1 + 2) * sizeof(WCHAR);
    wbuf = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, wbuf_len);
    if (!wbuf) goto done;

    /* Read and convert to UTF-8 */
    DWORD nread;
    if (!ReadConsoleW(hi, wbuf, len - 1 + 2, &nread, 0)) goto done;
    if (nread < 2) goto done;
    if (wbuf[nread - 2] != '\r' || wbuf[nread - 1] != '\n') goto done;
    wbuf[nread - 2] = 0;  // truncate "\r\n"
    pwlen = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, len, 0, 0);

done:
    if (wbuf) {
        SecureZeroMemory(wbuf, wbuf_len);
        HeapFree(GetProcessHeap(), 0, wbuf);
    }
    /* Exploit that operations on INVALID_HANDLE_VALUE are no-ops */
    WriteConsoleA(ho, "\n", 1, 0, 0);
    SetConsoleMode(hi, orig);
    CloseHandle(ho);
    CloseHandle(hi);
    return pwlen;
}

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC1 sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = NUM_BACK_BUFFERS;
        sd.Width = 0;
        sd.Height = 0;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;
    }

    // [DEBUG] Enable debug interface
#ifdef DX12_ENABLE_DEBUG_LAYER
    ID3D12Debug* pdx12Debug = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
        pdx12Debug->EnableDebugLayer();
#endif

    // Create device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
        return false;

    // [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
    if (pdx12Debug != nullptr)
    {
        ID3D12InfoQueue* pInfoQueue = nullptr;
        g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        pInfoQueue->Release();
        pdx12Debug->Release();
    }
#endif

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = NUM_BACK_BUFFERS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
            return false;

        SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        {
            g_mainRenderTargetDescriptor[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            return false;
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
            return false;
    }

    for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
            return false;

    if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK ||
        g_pd3dCommandList->Close() != S_OK)
        return false;

    if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
        return false;

    g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (g_fenceEvent == nullptr)
        return false;

    {
        IDXGIFactory4* dxgiFactory = nullptr;
        IDXGISwapChain1* swapChain1 = nullptr;
        if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
            return false;
        if (dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, nullptr, nullptr, &swapChain1) != S_OK)
            return false;
        if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
            return false;
        swapChain1->Release();
        dxgiFactory->Release();
        g_pSwapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
        g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, nullptr); g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_hSwapChainWaitableObject != nullptr) { CloseHandle(g_hSwapChainWaitableObject); }
    for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        if (g_frameContext[i].CommandAllocator) { g_frameContext[i].CommandAllocator->Release(); g_frameContext[i].CommandAllocator = nullptr; }
    if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = nullptr; }
    if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = nullptr; }
    if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = nullptr; }
    if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = nullptr; }
    if (g_fence) { g_fence->Release(); g_fence = nullptr; }
    if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

#ifdef DX12_ENABLE_DEBUG_LAYER
    IDXGIDebug1* pDebug = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
#endif
}

void CreateRenderTarget()
{
    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
    {
        ID3D12Resource* pBackBuffer = nullptr;
        g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, g_mainRenderTargetDescriptor[i]);
        g_mainRenderTargetResource[i] = pBackBuffer;
    }
}

void CleanupRenderTarget()
{
    WaitForLastSubmittedFrame();

    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = nullptr; }
}

void WaitForLastSubmittedFrame()
{
    FrameContext* frameCtx = &g_frameContext[g_frameIndex % NUM_FRAMES_IN_FLIGHT];

    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue == 0)
        return; // No fence was signaled

    frameCtx->FenceValue = 0;
    if (g_fence->GetCompletedValue() >= fenceValue)
        return;

    g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
    WaitForSingleObject(g_fenceEvent, INFINITE);
}

FrameContext* WaitForNextFrameResources()
{
    UINT nextFrameIndex = g_frameIndex + 1;
    g_frameIndex = nextFrameIndex;

    HANDLE waitableObjects[] = { g_hSwapChainWaitableObject, nullptr };
    DWORD numWaitableObjects = 1;

    FrameContext* frameCtx = &g_frameContext[nextFrameIndex % NUM_FRAMES_IN_FLIGHT];
    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue != 0) // means no fence was signaled
    {
        frameCtx->FenceValue = 0;
        g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
        waitableObjects[1] = g_fenceEvent;
        numWaitableObjects = 2;
    }

    WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

    return frameCtx;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            WaitForLastSubmittedFrame();
            CleanupRenderTarget();
            HRESULT result = g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
            assert(SUCCEEDED(result) && "Failed to resize swapchain.");
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}





//根据站点名字返回 标号
inline int to_point_index(string name) {
    return point_index[name];
}
//根据标号返回 站点名字
inline string to_point_name(int index) {
    return point_name[index];
}

void init() {
    //memset(dis, INF, sizeof(dis));
    //memset(vis, false, sizeof(vis));
    //memset(vis_func3, false, sizeof(vis_func3));
    for (int i = 0; i < line_count; i++) {
        line[i].clear();
    }
    point_index.clear();
    for (int i = 0; i < point_count; i++) {
        point_name[i].clear();
    }
    
    memset(is_change_point, 0, sizeof(is_change_point));

    remaining_point = -1;
    //consider_transfer_cost = 0;
    line_count = 0;
    point_count = 0;
    fun = 0;
    now_time = 0;
}


void input_map() {
    ifstream fin;
    if (city_map_flag == 0) {
        fin.open("beijingSubway.txt", ios::in);
    }
    else if (city_map_flag == 1) {
        fin.open("wuhanSubway.txt", ios::in);
    }
    string first_line;
    while (getline(fin, first_line)) {
        istringstream fls(first_line);
        getline(fls, line_name[line_count++], ' ');
        // 显示读入的路线名
        //cout << "line name : " << line_name[line_count - 1] << endl;
        line_start_time[line_count - 1] = 0;
        line_end_time[line_count - 1] = 23;
        string tmp;
        if (getline(fls, tmp, ' ')) {
            line_start_time[line_count - 1] = atoi(tmp.c_str());
        }
        if (getline(fls, tmp, ' ')) {
            line_end_time[line_count - 1] = atoi(tmp.c_str());
        }
        getline(fin, tmp);
        istringstream in(tmp);
        while (getline(in, tmp, ' ')) {
            // 如果这个车站是第一次出现，为它分配一个标号
            if (point_index.count(tmp) == 0) {
                point_index[tmp] = point_count;
                point_name[point_count++] = tmp;
                // 记录所属的路线
                belong_line[point_count - 1] = line_count - 1;
            }
            else {
                // 不是第一次出现，检测是否是换乘站
                int i = point_index[tmp];
                if (belong_line[i] != line_count - 1) {
                    is_change_point[i] = 1;
                }
            }
            int i = point_index[tmp];
            line[line_count - 1].push_back(i);

            // 显示读入的站名
            //cout << "|" << i << ' ' << tmp << "|";
        }
        //cout << endl;
    }
    //line_count--;
    fin.close();
}

//根据输入的 地铁线路名称 输出 该线路上的所有站点名称
string line_request(string name) {
    string res = "\n";
    int target_line = -1;
    for (int i = 0; i < line_count; ++i) {
        if (line_name[i] == name) {
            target_line = i;//取出线路名称对应的index号
            break;
        }
    }
    if (target_line == -1) {
        res = "\n对不起，您查询的路线不存在，请重新输入。\n";
    }
    else {
        for (int point : line[target_line]) {
            res += to_point_name(point) + "\n";
        }
    }

    return res;
}

string line_list() {
    string res = "\n" + line_name[0];
    for (int i = 1; i < line_count; ++i) {
        res += ", " + line_name[i];
    }
    return res + "\n";
}

//更新站点的邻接关系
void add_edge(int x, int y, int l, int c = 1) {
    neighbor[x].push_back(y);	//点y是点x的邻居
    cost[x].push_back(c);		//目前不考虑换线开销，故所有点连线的cost都默认为1
    from_line[x].push_back(l);	//x到y的路线是在哪条地铁线上
    neighbor[y].push_back(x);	//点x是点y的邻居
    cost[y].push_back(c);
    from_line[y].push_back(l);	//y到x的路线是在哪条地铁线上（当然与上面一致）
}

//初始化所有站点的邻接关系、站点连线所在的地铁线路（不考虑换乘开销）
void build_graph_normal() {
    for (int i = 0; i < point_count; ++i) {
        neighbor[i].clear();
        cost[i].clear();
        from_line[i].clear();
    }
    for (int i = 0; i < line_count; ++i) {//沿着不同的地铁线路来初始化在地铁线上的各个站点的邻接关系，并记录站点连线是几号线（即i）
        // 不在运营时间内，则此路线不连边
        if (now_time < line_start_time[i] || now_time > line_end_time[i]) {
            continue;
        }
        int pre_point = -1;//有效站点的index值从0开始
        for (int point : line[i]) {//按line[i]内站点的顺序，初始化i这条地铁线上各站点的信息
            if (pre_point >= 0) {//如果 pre_point 是有效站点
                add_edge(pre_point, point, i);//初始化
            }
            pre_point = point;//把这次循环初始化的站点置为前站点
        }
    }
}

// 计算路过站点数（func3调用时更新路过站点的vis_func3和剩余点数）
int count_step(int t, int s) {
    int temp = t;
    int count = 1;
    int last_line = -1;
    while (temp != s) {
        count++;

        if (consider_transfer_cost == 1) {// 计算换乘开销
            if (pre_line[temp] != last_line && last_line != -1) {// 如果和上次坐的线不同了就要额外加count
                count += 3;
            }
            last_line = pre_line[temp];
        }

        temp = pre[temp];

        //func3标记更新，非func3调用此函数时不执行
        if (vis_func3[temp] == false) {
            vis_func3[temp] = true;
            remaining_point--;
        }

    }

    return count;
}

// ##函数重载##计算路过站点数，不更新路过站点
int count_step(int t, int s, bool global) {
    global = 1;
    int temp = t;
    int count = 1;
    int last_line = -1;
    while (temp != s) {
        count++;

        if (consider_transfer_cost == 1) {// 计算换乘开销
            if (pre_line[temp] != last_line && last_line != -1) {// 如果和上次坐的线不同了就要额外加count
                count += 3;
            }
            last_line = pre_line[temp];
        }

        temp = pre[temp];
    }


    return count;
}



//输入起点终点生成输出路线string（调用该函数的前提是进行过bfs）
string route(int s, int t) {
    string res = "";
    int temp = t;
    int last_line = -1;
    do {
        if (temp != t) {
            res += to_point_name(t);//string上加站点名称
        }

        if (fun == 2) {
            if (pre_line[t] != last_line && last_line != -1) {//如果和上次坐的线不同了就要输出换乘信息
                res += "  换乘" + line_name[pre_line[t]];
            }
        }

        last_line = pre_line[t];
        if (fun == 1 || fun == 2) {
           res += "\n";
        }
        else if (fun == 3) {
            if (temp != t) {
                res += "\n";
            }
        }


        t = pre[t];
    } while (t != s);
    res += to_point_name(t);

    return res + "\n";
}

//bfs_func3不更新func3内的数据，在count_step函数中更新func3内的访问标记和剩余点数。
string bfs_func3(int point_now, int next_point) {
    int t = point_now, s = next_point;
    queue<int> q;//先进先出
    memset(vis, 0, sizeof(vis));
    q.push(s);//终点站序号入队
    dis[s] = 0;//把自己到自己的距离更新为0
    vis[s] = true;//标记此点，说明已经走过
    while (!q.empty()) {
        int point = q.front();//取队列第一个数的值
        q.pop();//删除队列第一个数
        for (int i = 0; i < (int)neighbor[point].size(); ++i) {
            int nex_point = neighbor[point][i];
            if (!vis[nex_point]) {//没走过该点
                vis[nex_point] = 1;//标记此点
                dis[nex_point] = dis[point] + 1;
                pre[nex_point] = point;
                pre_line[nex_point] = from_line[point][i];
                if (nex_point == t) {
                    break;//到达终点break
                }
                q.push(nex_point);
            }
        }
        if (vis[t]) break;
    }
    return route(s, t);
}


int quick_sort(int* dis_global, int left, int right, int* point_rank) {

    if (left > right) {
        return -1;
    }

    int pivot = dis_global[left];//在数组最左边的距离
    int point_trank_temp = point_rank[left];

    int i = left;//i左
    int j = right;//j右

    while (i != j) {
        while (i < j && dis_global[j] >= pivot) {
            j--;
        }
        dis_global[i] = dis_global[j];
        point_rank[i] = point_rank[j];

        while (i < j && dis_global[i] <= pivot) {
            i++;
        }
        dis_global[j] = dis_global[i];
        point_rank[j] = point_rank[i];
    }

    dis_global[i] = pivot;
    point_rank[i] = point_trank_temp;

    quick_sort(dis_global, left, i - 1, point_rank);
    quick_sort(dis_global, i + 1, right, point_rank);

    return 0;
}

//取随机浮点数
double getRandData(int min, int max) {
    double m1 = (double)(rand() % 101) / 101;                        // 计算 0，1之间的随机小数,得到的值域近似为(0,1)
    min++;                                                                             //将 区间变为(min+1,max),
    double m2 = (double)((rand() % (max - min + 1)) + min);    //计算 min+1,max 之间的随机整数，得到的值域为[min+1,max]
    m2 = m2 - 1;                                                                        //令值域为[min,max-1]
    return m1 + m2;                                                                //返回值域为(min,max),为所求随机浮点数
}



//功能一：查询并输出路线上的站点
int task_line_req() {
    while (true) {
        cout << "输入0退出，输入?显示所有路线。" << endl;
        cout << "请输入想要查询的路线：";
        cout.flush();

        char tmp_c[300];
        read_password(tmp_c, 300, "");
        string tmp = tmp_c;
        if (tmp == "0") break;
        if (tmp == "?" || tmp == "？") {
            cout << line_list() << endl;
        }
        else {
            cout << line_request(tmp) << endl;
        }
    }
    return 0;
}

//功能二：计算两个车站之间的最短距离（换乘不额外计距离）。返回需要输出的字符串。因为相邻车站之间的距离都为1，使用BFS算法。 
string display_normal(string src_name, string tar_name) {
    fun = 2;
    int t = to_point_index(src_name), s = to_point_index(tar_name);//！！！t是起点，s是终点（与后续程序命名冲突，后续修改）
    if (s == t) {//如果起点就是终点
        return "\n" + (string)"1" + "\n" + point_name[t] + "\n\n";
    }
    build_graph_normal();//建图
    queue<int> q;//先进先出
    q.push(s);//终点站序号入队
    dis[s] = 0;//把自己到自己的距离更新为0
    vis[s] = true;//标记此点，说明已经走过
    while (!q.empty()) {
        int point = q.front();//取队列第一个数的值
        q.pop();//删除队列第一个数
        for (int i = 0; i < (int)neighbor[point].size(); ++i) {
            int nex_point = neighbor[point][i];
            if (!vis[nex_point]) {//没走过该点
                vis[nex_point] = 1;//标记此点
                dis[nex_point] = dis[point] + 1;
                pre[nex_point] = point;
                pre_line[nex_point] = from_line[point][i];
                if (nex_point == t) {
                    break;//到达终点break
                }
                q.push(nex_point);
            }
        }
        if (vis[t]) break;
    }

    string res = "\n";
    int count_point = count_step(t, s);
    res = res + to_string(count_point) + "\n" + point_name[t] + route(s, t);
    return res;
}


//功能二（求两点路径）函数的"重载"，但是返回vector<int>
vector<int> display_normal_vi(int src, int tar) {
    fun = 2;
    int t = src, s = tar;//！！！t是起点，s是终点（与后续程序命名冲突，后续修改）
    if (s == t) {//如果起点就是终点
        vector<int> v;
        v.push_back(0);
        v.push_back(s);
        return v;
    }
    build_graph_normal();//建图
    memset(dis, INF, sizeof(dis));
    memset(vis, false, sizeof(vis));
    queue<int> q;//先进先出
    q.push(s);//终点站序号入队
    dis[s] = 0;//把自己到自己的距离更新为0
    vis[s] = true;//标记此点，说明已经走过
    while (!q.empty()) {
        int point = q.front();//取队列第一个数的值
        q.pop();//删除队列第一个数
        for (int i = 0; i < (int)neighbor[point].size(); ++i) {
            int nex_point = neighbor[point][i];
            if (!vis[nex_point]) {//没走过该点
                vis[nex_point] = 1;//标记此点
                dis[nex_point] = dis[point] + 1;
                pre[nex_point] = point;
                pre_line[nex_point] = from_line[point][i];
                if (nex_point == t) {
                    break;//到达终点break
                }
                q.push(nex_point);
            }
        }
        if (vis[t]) break;
    }



    vector<int> v;
    while (t != s) {
        v.push_back(t);
        t = pre[t];
    }
    v.push_back(s);
    return v;
}


// 功能三：计算尽可能快地遍历地铁的所有车站的路线
string display_func3(string src_name) {

    //如果不考虑换乘开销就注释掉本条
    //consider_transfer_cost = 1;
    //

    fun = 3;
    string res;//最终路线结果
    string res_temp;//记录经过站点路线
    int step = INF;//最终步数结果
    int step_temp = INF;//记录经过站点数
    int st = to_point_index(src_name);//起点
    build_graph_normal();//建图

    //重复nn次取最短路径，输出路径最小的一次
    for (int nn = 20; nn > 0; nn--) {


        step_temp = 1;
        st = to_point_index(src_name);
        res_temp = point_name[st] + "\n";
        int point_now = st;//此时所在点
        int next_point = MAX_POINT - 1;//bfs目标点
        remaining_point = point_count;


        remaining_point--;		//
        vis_func3[st] = true;	//排除起点
        //

        while (remaining_point != 0) {	// 优先从 point_now 的 neighbor 中随机选一个 vis_func3 != true 的点作为下一点
            int neighbor_size = (int)neighbor[point_now].size();//邻接点个数
            int* point_unreached_adjacency = new int[neighbor_size];
            int j = 0;	//point_unreached的下标
            for (int i = 0; i < neighbor_size; i++) {
                if (vis_func3[neighbor[point_now][i]] != true) {
                    point_unreached_adjacency[j] = neighbor[point_now][i];
                    j++;
                }
            }


            if (j != 0) {	//j != 0 说明point_unreached_adjacency不为空
                //取随机数
                srand((unsigned int)time(NULL));
                int temp = rand() % j;
                next_point = point_unreached_adjacency[temp];

                res_temp += to_point_name(next_point) + "\n";

                vis_func3[next_point] = true;
                point_now = next_point;
                remaining_point--;

                step_temp++;

                delete[] point_unreached_adjacency;
            }
            else {		//point_unreached_adjacency为空，说明邻接点都被标记了，从全局找 next_point
                int* point_unreached_global = new int [500];
                int k = 0;//point_unreached_2的下标
                for (int i = 0; i < point_count; i++) {
                    if (vis_func3[i] != true) {
                        point_unreached_global[k] = i;
                        k++;
                    }
                }

                // 算法1：全局中完全随机地选取下一点
                /*
                srand((unsigned int)time(NULL));
                int temp = rand() % k;
                next_point = point_unreached_global[temp];
                */



                // 算法2：退火算法，根据权重随机选取全局中未走过的一点作为下一跳，距离越近权重越大
                // 某个全局点：point_unreached_global[i]，i∈[0,k)
                // 将该全局点作为下一点，计算最短距离，并记录到dis_global[i]中
                // 从dis_global中找到最小距离，取对应i，得对应点point_unreached_global[i]
                int* dis_global = new int[k];
                {
                    queue<int> tmpq;
                    int tmpdis[MAX_POINT];
                    memset(tmpdis, 0x7f, sizeof(tmpdis));
                    tmpq.push(point_now);
                    tmpdis[point_now] = 0;
                    while(tmpq.size()) {
                        int x = tmpq.front();
                        tmpq.pop();
                        for(int y:neighbor[x]) {
                            if(tmpdis[y] > tmpdis[x] + 1) {
                                tmpdis[y] = tmpdis[x] + 1;
                                tmpq.push(y);
                            }
                        }
                    }
                    for(int i=0;i<k;++i) {
                        dis_global[i] = tmpdis[point_unreached_global[i]];
                    }
                }
                // 把dis_global中的数据按照从小到大排序，排在前的权重大，排在后的权重小
                // 创建一个point_rank跟踪点编号i，初始化point_rank[i] = i
                // 快排交换dis_global中的距离的同时交换point_rank中的编号
                int* point_rank = new int[k];
                for (int i = 0; i < k; i++) {
                    point_rank[i] = i;
                }
                quick_sort(dis_global, 0, k - 1, point_rank);
                // 通过把point_rank[i]作为数组系数即可按照路径距离从小到大访问point_unreached_global
                // 排在point_rank前面的权重大，按照数组系数i计算权重后随机选取一点
                // 设选中point_rank[i]的概率P= e^(-i)/[e^(-0)+e^(-1)+...+e^(-k+1)]
                // 随机数落在区间0 ~ e^(-0)时取0，落在e^(-0) ~ e^(-0)+e^(-1)时取1，后类似
                double* P = new double[k];
                double Psum = 0;
                int const_num = 100; // 退火算法常数取值至关重要，多测试几个常数下的算法性能
                for (int i = 0; i < k; i++) {
                    P[i] = pow(2.17828, -const_num * dis_global[i]);
                    Psum += P[i];
                }

                srand((unsigned int)time(NULL));
                double random_num = getRandData(0, const_num);
                random_num = random_num * Psum / const_num;
                next_point = point_unreached_global[0];
                for (int i = 0; i < k; i++) {
                    random_num -= P[i];
                    if (random_num <= 0) {
                        next_point = point_unreached_global[point_rank[i]];
                        break;
                    }
                }
                // 通过算法选定下一跳站点后调用bfs
                res_temp += bfs_func3(point_now, next_point);

                step_temp += (count_step(point_now, next_point) - 1);
                remaining_point;
                vis_func3[next_point] = true;
                point_now = next_point;

                delete[] point_unreached_global;
                delete[] dis_global;
                delete[] point_rank;
                delete[] P;
                // 初始化所有func2中使用的所有全局变量
                memset(vis, false, sizeof(vis));
                memset(dis, INF, sizeof(dis));
                memset(pre, 0, sizeof(pre));
                memset(pre_line, 0, sizeof(pre_line));
            }
        }

        // 所有点走完了，回起点
        if (remaining_point == 0) {

            next_point = st;
            // 调用bfs
            bfs_func3(point_now, next_point);
            res_temp += bfs_func3(point_now, next_point);

            step_temp += (count_step(point_now, next_point) - 1);
            remaining_point;
            vis_func3[next_point] = true;
            point_now = next_point;
        }


        if (step_temp < step) {
            res.clear();
            res = res_temp;
            step = step_temp;
        }

        res_temp.clear();

        //memset(neighbor, NULL, sizeof(neighbor));
        //memset(cost, NULL, sizeof(cost));
        //memset(from_line, NULL, sizeof(from_line));
        memset(vis, false, sizeof(vis));
        memset(dis, INF, sizeof(dis));
        memset(pre, 0, sizeof(pre));
        memset(pre_line, 0, sizeof(pre_line));
        memset(vis_func3, false, sizeof(vis_func3));
    }


    return to_string(step) + "\n" + res;
    //return to_string(step) + "\n";
}


// 功能三（求遍历路径）函数的"重载"，但是返回vector<int>
vector<int> display_func3_vi(string src_name) {

    //如果不考虑换乘开销就注释掉本条
    //consider_transfer_cost = 1;
    //

    fun = 3;
    string res;//最终路线结果
    string res_temp;//记录经过站点路线
    int step = INF;//最终步数结果
    int step_temp = INF;//记录经过站点数
    int st = to_point_index(src_name);//起点
    build_graph_normal();//建图

    // 重复nn次取最短路径，输出路径最小的一次
    for (int nn = 20; nn > 0; nn--) {


        res_temp = src_name + "\n";
        step_temp = 1;
        st = to_point_index(src_name);
        int point_now = st;//此时所在点
        int next_point = MAX_POINT - 1;//bfs目标点
        bool * visible = (bool*)malloc(point_count * sizeof(bool));
        remaining_point = 0;
        memset(visible, 0, sizeof (bool) * point_count);
        for (int i = 0; i < line_count; ++i) {
            if (now_time < line_start_time[i] || now_time > line_end_time[i]) continue;
            //cout << "line : " << line_name[i] << " in" << endl;
            for (int x : line[i]) {
                //cout << "x = " << x << "visible[x] = " << visible[x] << endl;
                if (!visible[x]) {
                    visible[x] = true;
                    ++remaining_point;
                    //cout << "remain + 1 = " << remaining_point << endl;
                }
            }
        }
        //printf_s("remain point = %d \n", remaining_point);

        remaining_point--;		//
        vis_func3[st] = true;	//排除起点
        //

        while (remaining_point != 0) {	// 优先从 point_now 的 neighbor 中随机选一个 vis_func3 != true 的点作为下一点
            int neighbor_size = (int)neighbor[point_now].size();//邻接点个数
            int* point_unreached_adjacency = new int[neighbor_size];
            int j = 0;	//point_unreached的下标
            for (int i = 0; i < neighbor_size; i++) {
                int x = neighbor[point_now][i];
                if (!vis_func3[x] && visible[x]) {
                    point_unreached_adjacency[j] = x;
                    j++;
                }
            }


            if (j != 0) {	//j != 0 说明point_unreached_adjacency不为空
                //取随机数
                srand((unsigned int)time(NULL));
                int temp = rand() % j;
                next_point = point_unreached_adjacency[temp];

                res_temp += to_point_name(next_point) + "\n";

                vis_func3[next_point] = true;
                point_now = next_point;
                remaining_point--;

                step_temp++;

                delete[] point_unreached_adjacency;
            }
            else {		//point_unreached_adjacency为空，说明邻接点都被标记了，从全局找 next_point
                int* point_unreached_global = new int[point_count];
                int k = 0;//point_unreached_2的下标
                for (int i = 0; i < point_count; i++) {
                    if (!vis_func3[i] && visible[i]) {
                        point_unreached_global[k] = i;
                        k++;
                    }
                }

                // 算法1：全局中完全随机地选取下一点
                
                //srand((unsigned int)time(NULL));
                //int temp = rand() % k;
                //next_point = point_unreached_global[temp];
                



                // 算法2：退火算法，根据权重随机选取全局中未走过的一点作为下一跳，距离越近权重越大
                // 某个全局点：point_unreached_global[i]，i∈[0,k)
                // 将该全局点作为下一点，计算最短距离，并记录到dis_global[i]中
                // 从dis_global中找到最小距离，取对应i，得对应点point_unreached_global[i]
                int* dis_global = new int[k];
                for (int i = 0; i < k; i++) {
                    int next_point_global = point_unreached_global[i];

                    bfs_func3(point_now, next_point_global);

                    dis_global[i] = count_step(point_now, next_point_global, true);

                    memset(vis, false, sizeof(vis));
                    memset(dis, INF, sizeof(dis));
                    memset(pre, 0, sizeof(pre));
                    memset(pre_line, 0, sizeof(pre_line));
                }
                // 把dis_global中的数据按照从小到大排序，排在前的权重大，排在后的权重小
                // 创建一个point_rank跟踪点编号i，初始化point_rank[i] = i
                // 快排交换dis_global中的距离的同时交换point_rank中的编号
                int* point_rank = new int[k];
                for (int i = 0; i < k; i++) {
                    point_rank[i] = i;
                }
                quick_sort(dis_global, 0, k - 1, point_rank);
                // 通过把point_rank[i]作为数组系数即可按照路径距离从小到大访问point_unreached_global
                // 排在point_rank前面的权重大，按照数组系数i计算权重后随机选取一点
                // 设选中point_rank[i]的概率P= e^(-i)/[e^(-0)+e^(-1)+...+e^(-k+1)]
                // 随机数落在区间0 ~ e^(-0)时取0，落在e^(-0) ~ e^(-0)+e^(-1)时取1，后类似
                double* P = new double[k];
                double Psum = 0;
                int const_num = 100; // 退火算法常数取值至关重要，多测试几个常数下的算法性能
                for (int i = 0; i < k; i++) {
                    P[i] = pow(2.17828, -const_num * dis_global[i]);
                    Psum += P[i];
                }

                srand((unsigned int)time(NULL));
                double random_num = getRandData(0, const_num);
                random_num = random_num * Psum / const_num;

                for (int i = 0; i < k; i++) {
                    random_num -= P[i];
                    if (random_num <= 0) {
                        next_point = point_unreached_global[point_rank[i]];
                        break;
                    }
                }
                // 通过算法选定下一跳站点后调用bfs
                res_temp += bfs_func3(point_now, next_point);

                step_temp += (count_step(point_now, next_point) - 1);
                remaining_point;
                vis_func3[next_point] = true;
                point_now = next_point;

                delete[] point_unreached_global;
                delete[] dis_global;
                delete[] point_rank;
                delete[] P;
                // 初始化所有func2中使用的所有全局变量
                memset(vis, false, sizeof(vis));
                memset(dis, INF, sizeof(dis));
                memset(pre, 0, sizeof(pre));
                memset(pre_line, 0, sizeof(pre_line));
            }
        }

        // 所有点走完了，回起点
        if (remaining_point == 0) {

            next_point = st;
            // 调用bfs
            bfs_func3(point_now, next_point);
            res_temp += bfs_func3(point_now, next_point);

            step_temp += (count_step(point_now, next_point) - 1);
            remaining_point;
            vis_func3[next_point] = true;
            point_now = next_point;
        }


        if (step_temp < step) {
            res.clear();
            res = res_temp;
            step = step_temp;
        }

        res_temp.clear();

        //memset(neighbor, NULL, sizeof(neighbor));
        //memset(cost, NULL, sizeof(cost));
        //memset(from_line, NULL, sizeof(from_line));
        memset(vis, false, sizeof(vis));
        memset(dis, INF, sizeof(dis));
        memset(pre, 0, sizeof(pre));
        memset(pre_line, 0, sizeof(pre_line));
        memset(vis_func3, false, sizeof(vis_func3));
    }

    remaining_steps = step;
    char s[MAX_POINT*MAX_POINT];
    char *buf = new char[MAX_POINT*MAX_POINT];
    strcpy_s(s, MAX_POINT*MAX_POINT, res.c_str());//strcpy(s, res.c_str());
    char* p = strtok_s(s, "\n", &buf);
    //vector<string> point_name_vi;
    while (p) {
        //point_name_vi.push_back(p);
        v_func3_vi.push_back(to_point_index(p));

        p = strtok_s(NULL, "\n", &buf);
    }

 
    return v_func3_vi;
    //return to_string(step) + "\n" + res;
}


// --------------------------------GUI部分----------------------------------------

ImVec2 point_pos[MAX_POINT];
ImColor line_color[MAX_LINE];
const ImU32 black = ImColor(0, 0, 0);
const ImU32 grey = ImColor(62, 62, 66);
const ImU32 disable_color = ImColor(175, 175, 186);
const ImU32 white = ImColor(255, 255, 255);

const int window_size_x = GetSystemMetrics(SM_CXSCREEN);
const int window_size_y = GetSystemMetrics(SM_CYSCREEN);
const int scale_x = window_size_x / 56, scale_y = window_size_y / 56, radius = window_size_x / 340, radius_small = window_size_x / 600, thk = 4, font_size = window_size_x / 150;

int used_edge[MAX_POINT*MAX_POINT][2];
int used_edge_count;

// 初始化gui全局变量
void init_gui() {
    memset(line_color, 0, sizeof(line_color));
    memset(point_pos, 0, sizeof(point_pos));
    memset(used_edge, 0, sizeof(used_edge));
    used_edge_count = 0;
    return;
}

// 读入车站的座标、路线的颜色
void input2() {
    ifstream fin;
    if (city_map_flag == 0) {
        fin.open("beijingSubwayMap.txt", ios::in);
    }
    else if (city_map_flag == 1) {
        fin.open("wuhanSubwayMap.txt", ios::in);
    }
    std::string buff, tmp;
    // 先读入路线颜色
    for (int i = 0; i < line_count; ++i) {
        getline(fin, buff);
        //printf_s("i = %d buff = ", i); cout << buff << endl;
        istringstream in(buff);
        getline(in, tmp, ' ');
        // tmp = name
        for (int j = 0; j < line_count; ++j) {
            if (line_name[j] == tmp) {
                getline(in, tmp, ' ');
                int r = atoi(tmp.c_str());
                getline(in, tmp, ' ');
                int g = atoi(tmp.c_str());
                getline(in, tmp, ' ');
                int b = atoi(tmp.c_str());
                line_color[j] = ImColor(r, g, b);
                //cout << "line " << line_name[j]; printf_s(" color: %d %d %d\n", r, g, b);
                break;
            }
        }
    }
    // 再读入车站座标
    while (getline(fin, buff)) {
        // 显示读入的路线名
        // cout << "line name : " << line_name[line_count - 1] << endl;
        istringstream in(buff);
        getline(in, tmp, ' ');
        // tmp = name
        int p_id = to_point_index(tmp);
        getline(in, tmp, ' ');
        int y = atoi(tmp.c_str());
        getline(in, tmp, ' ');
        int x = atoi(tmp.c_str());
        point_pos[p_id] = ImVec2((float)x * scale_x, (float)y * scale_y);
        //cout << to_point_name(p_id); printf_s(" i=%d | %d %d | %d\n", p_id, x, y, is_change_point[p_id]);
    }
    fin.close();
}

// 这条边是否正在展示并且被走过
bool is_edge_used(int x, int y) {
    for (int i = 0; i < used_edge_count; ++i) {
        int p = used_edge[i][0], q = used_edge[i][1];
        if ((x + y == p + q) && (x == p || x == q)) {
            return true;
        }
    }
    return false;
}



// Main code
int main(int argc, char* argv[]) {

    //调试哪个功能就对应哪个标识符：
    
    //argc = 4;
    //argv[1] = const_cast<char*>("/g");
    //argv[2] = const_cast<char*>("良乡大学城北");//起点
    //argv[3] = const_cast<char*>("北京站");//目的地
    //


    system("chcp 65001");
    setlocale(LC_ALL, ".UTF-8");
    //输入 n m,初始化各点距离，相连的点距离为1，未相连的点距离为INF,
    init();
    input_map();
    input2();

    //功能一：查询路线，输出路线上所有站点
    now_time = 12;
    if (argc == 1) {

        fun = 1;\
        return task_line_req();

    }

    //功能二：计算所输入两个车站间的最短路线
    if (strcmp(argv[1], "/b") == 0) {

        fun = 2;
        cout << display_normal((string)argv[2], (string)argv[3]);
        
    }

    //功能三：计算尽可能快地遍历地铁的所有车站的路线(先不考虑换乘开销，地图为等权无向图）
    if (strcmp(argv[1], "/a") == 0) {

        fun = 3;
        cout << display_func3((string)argv[2]);


    }

    //功能四：图形化界面
    if (strcmp(argv[1], "/g") == 0) {

        fun = 4;
        // Create application window
        //ImGui_ImplWin32_EnableDpiAwareness();


        WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"subway", nullptr };
        ::RegisterClassExW(&wc);
        HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"地铁换乘系统", WS_OVERLAPPEDWINDOW, 1, 1, window_size_x, window_size_y, nullptr, nullptr, wc.hInstance, nullptr);

        // Initialize Direct3D
        if (!CreateDeviceD3D(hwnd)) {
            CleanupDeviceD3D();
            ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return 1;
        }

        // Show the window
        ::ShowWindow(hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(hwnd);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        //ImGui::StyleColorsDark();
        ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX12_Init(g_pd3dDevice, NUM_FRAMES_IN_FLIGHT,
            DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap,
            g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
            g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("font.ttf", 18.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        ImFont* font = io.Fonts->AddFontFromFileTTF("font.ttf", (float)font_size, nullptr, io.Fonts->GetGlyphRangesChineseFull());
        ImFont* font_city_name = io.Fonts->AddFontFromFileTTF("font.ttf", (float)font_size+25, nullptr, io.Fonts->GetGlyphRangesChineseFull());

        IM_ASSERT(font != nullptr);

        // Our state
        bool show_demo_window = true;
        //bool show_another_window = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        bool is_route_displaying = false, is_flick = false;
        vector<int> display_route;
        int display_now_point=0;
        clock_t display_timer=clock(), flick_timer=clock();

        // Main loop
        bool done = false;
        while (!done) {
            // Poll and handle messages (inputs, window resize, etc.)
            // See the WndProc() function below for our to dispatch events to the Win32 backend.
            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done)
                break;

            // Start the Dear ImGui frame
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            //if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
            /*
            {
                static float f = 0.0f;
                static int counter = 0;

                ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

                ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
                ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
                ImGui::Checkbox("Another Window", &show_another_window);

                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

                if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                ImGui::End();
            }
            */
            // show map
            {
                const int scx = scale_x, scy = scale_y, r = radius, rr = radius_small;


                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs;
                ImGui::SetNextWindowPos(ImVec2(-2, 0));
                ImGui::SetNextWindowSize(ImVec2((float)200 * scx, (float)200 * scy));
                ImGui::Begin("map", (bool*)0, window_flags);
                ImGui::PushItemWidth(-ImGui::GetFontSize() * 15);
                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                // Draw background
                draw_list->AddRectFilled(ImVec2(-2.0, 0.0), ImVec2((float)200 * scx, (float)200 * scy), white);

                // draw lines
                for (int i = 0; i < line_count; ++i) {
                    int last_point = -1;
                    ImColor lcol = (now_time < line_start_time[i] || now_time > line_end_time[i]) ? disable_color : line_color[i];
                    //cout << "draw line : " << line_name[i] << " : " << line[i].size() << endl;
                    for (auto now_point : line[i]) {
                        if (last_point != -1) {
                            // link two points
                            ImColor col = 
                                (is_route_displaying && is_edge_used(last_point, now_point)) ? grey : lcol;
                            if (last_point + now_point == 69 && (last_point == 34 || last_point == 35)) {
                                // 34 & 35
                                ImVec2 p = point_pos[last_point], q = point_pos[now_point];
                                int d = 2;
                                if (i == 1)
                                    draw_list->AddLine(ImVec2(p.x + d, p.y + d), ImVec2(q.x + d, q.y + d), col, thk);
                                else
                                    draw_list->AddLine(ImVec2(p.x - d, p.y - d), ImVec2(q.x - d, q.y - d), col, thk);
                            }
                            else {
                                draw_list->AddLine(point_pos[last_point], point_pos[now_point], col, thk);
                            }
                        }
                        last_point = now_point;
                    }
                }

                // draw points
                for (int i = 0; i < point_count; ++i) {
                    ImU32 col = white;
                    if (is_route_displaying) {
                        for (int j = 0; j < display_now_point; ++j) {
                            if (display_route[j] == i) {
                                col = grey;
                                break;
                            }
                        }
                        if (display_route[display_now_point] == i && is_flick) {
                            col = ImColor(0, 122, 204);
                        }
                    }
                    draw_list->AddCircleFilled(point_pos[i], (float)r, col);
                    draw_list->AddCircle(point_pos[i], (float)r, black);
                    if (is_change_point[i]) {
                        draw_list->AddCircle(point_pos[i], (float)rr, black);
                    }
                }



                //ImGui::Dummy(ImVec2((sz + spacing) * 10.2f, (sz + spacing) * 3.0f));
                ImGui::Dummy(ImVec2((float)200 * scx, (float)200 * scy));
                ImGui::PopItemWidth();
                ImGui::End();
            }
            // add name
            {
                const int scx = scale_x, scy = scale_y, r = radius, rr = radius_small;
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs;
                //ImGuiWindowFlags window_flags = 0;
                for (int i = 0; i < point_count; ++i) {
                    ImGui::SetNextWindowPos(ImVec2(point_pos[i].x - 15, point_pos[i].y - 4));
                    ImGui::SetNextWindowSize(ImVec2((float)10 * scx, (float)10 * scy));
                    char buff[20];
                    sprintf_s(buff, "point_label%d", i);
                    ImGui::Begin(buff, (bool*)0, window_flags);
                    ImGui::Text(point_name[i].c_str());
                    ImGui::End();
                }

            }

            // display city name
            {
                const int scx = scale_x, scy = scale_y, r = radius, rr = radius_small;
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs;
                ImGui::SetNextWindowPos(ImVec2((float)((double)window_size_x-130.0), (float)20.0));

                ImGui::Begin("display city name", (bool*)0, window_flags);

                ImGui::PushFont(font_city_name);
                if (city_map_flag == 0) 
                    ImGui::Text(u8"北京");
                else if (city_map_flag == 1) 
                    ImGui::Text(u8"武汉");
                ImGui::PopFont();

                ImGui::End();
            }


            
            // 3. Show selector of start and end point

            {
                const int space = (int)ImGui::GetTextLineHeightWithSpacing();
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize;
                // ImGui::SetNextWindowSize(ImVec2(16*space, 16*space));
                ImGui::Begin("op&ed select", (bool*)0, window_flags);

                static int start_line = 0, end_line = 0, start_point = 0, end_point = 0; // Here we store our selection data as an index.
                ImGui::SeparatorText("当前时间");
                ImGui::SliderInt("##select_time", &now_time, 0, 23);
                ImGui::SeparatorText("选择起点");
                if (ImGui::BeginListBox("##1", ImVec2((float)space * 7, (float)space * 5))) {
                    for (int i = 0; i < line_count; i++) {
                        if (now_time < line_start_time[i] || now_time > line_end_time[i]) {
                            continue;
                        }
                        const bool is_selected = (start_line == i);
                        if (ImGui::Selectable(line_name[i].c_str(), is_selected))
                            start_line = i;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndListBox();
                }
                ImGui::SameLine();
                if (ImGui::BeginListBox("##2", ImVec2((float)space * 7, (float)space * 5))) {
                    for (int i = 0; i < (int)line[start_line].size(); i++) {
                        const bool is_selected = (start_point == i);
                        if (ImGui::Selectable(point_name[line[start_line][i]].c_str(), is_selected))
                            start_point = i;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndListBox();
                }
                ImGui::SeparatorText("选择终点");
                if (ImGui::BeginListBox("##3", ImVec2((float)space * 7, (float)space * 5))) {
                    for (int i = 0; i < line_count; i++) {
                        if (now_time < line_start_time[i] || now_time > line_end_time[i]) {
                            continue;
                        }
                        const bool is_selected = (end_line == i);
                        if (ImGui::Selectable(line_name[i].c_str(), is_selected))
                            end_line = i;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndListBox();
                }
                ImGui::SameLine();
                if (ImGui::BeginListBox("##4", ImVec2((float)space * 7, (float)space * 5))) {
                    for (int i = 0; i < (int)line[end_line].size(); i++) {
                        const bool is_selected = (end_point == i);
                        if (ImGui::Selectable(point_name[line[end_line][i]].c_str(), is_selected))
                            end_point = i;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndListBox();
                }
                if (ImGui::Button("查询路线", ImVec2(-FLT_MIN, (float)space * 2))) {

                    cout << "start : " << point_name[line[start_line][start_point]] << endl;
                    cout << "end   : " << point_name[line[end_line][end_point]] << endl;
                    int start = line[start_line][start_point];
                    int end = line[end_line][end_point];
                    if (start != end) {
                        is_route_displaying = true;
                        used_edge_count = 0;
                        display_route = display_normal_vi(start, end);
                        display_now_point = 0;
                        display_timer = clock();
                        flick_timer = clock();
                    }
                }
                if (is_route_displaying) {

                    if (clock() - flick_timer > 500) {
                        is_flick = !is_flick;
                        flick_timer = clock();
                    }
                    if (clock() - display_timer > 2000) {
                        display_now_point++;
                        display_timer = clock();
                        if (display_now_point == (int)display_route.size()) {
                            is_route_displaying = false;
                            --display_now_point;
                        }
                        else {
                            used_edge[used_edge_count][0] = display_route[display_now_point];
                            used_edge[used_edge_count][1] = display_route[display_now_point - 1];
                            ++used_edge_count;
                        }
                    }
                   //int len = (int)display_route.size() - 1;
                   // ImGui::Text(count_step);
                    
                }
                ImGui::End();
            }
            



            // 4. Show selector of start point（全图遍历）

            {
                const int space = (int)ImGui::GetTextLineHeightWithSpacing();
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize;
                // ImGui::SetNextWindowSize(ImVec2(16*space, 16*space));
                ImGui::Begin("op&ed select", (bool*)0, window_flags);

                static int all_line = 0,  all_point = 0; // Here we store our selection data as an index.

                ImGui::SeparatorText("选择全图遍历起点");
                if (ImGui::BeginListBox("##5", ImVec2((float)space * 7, (float)space * 5))) {
                    for (int i = 0; i < line_count; i++) {
                        if (now_time < line_start_time[i] || now_time > line_end_time[i]) {
                            continue;
                        }
                        const bool is_selected = (all_line == i);
                        if (ImGui::Selectable(line_name[i].c_str(), is_selected))
                            all_line = i;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndListBox();
                }
                ImGui::SameLine();
                if (ImGui::BeginListBox("##6", ImVec2((float)space * 7, (float)space * 5))) {
                    for (int i = 0; i < (int)line[all_line].size(); i++) {
                        const bool is_selected = (all_point == i);
                        if (ImGui::Selectable(point_name[line[all_line][i]].c_str(), is_selected))
                            all_point = i;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndListBox();
                }
                if (ImGui::Button("查询遍历路线", ImVec2(-FLT_MIN, (float)space * 2))) {

                    cout << "start : " << point_name[line[all_line][all_point]] << endl;
                    int start = line[all_line][all_point];

                    v_func3_vi.clear();
                    is_route_displaying = true;
                    used_edge_count = 0;
                    display_route = display_func3_vi(to_point_name(start));
                    display_now_point = 0;
                    display_timer = clock();
                    flick_timer = clock();
                    
                }
                if (is_route_displaying) {

                    if (clock() - flick_timer > 100) {
                        is_flick = !is_flick;
                        flick_timer = clock();
                    }
                    if (clock() - display_timer > 200) {

                        remaining_steps--;

                        display_now_point++;
                        display_timer = clock();
                        if (display_now_point == (int)display_route.size()) {
                            is_route_displaying = false;
                            --display_now_point;
                        }
                        else {
                            used_edge[used_edge_count][0] = display_route[display_now_point];
                            used_edge[used_edge_count][1] = display_route[display_now_point - 1];
                            ++used_edge_count;
                        }
                    }
                    int len = (int)display_route.size() - 1;
                    ImGui::Text(point_name[display_route[0]].c_str());
                    ImGui::SameLine();
                    ImGui::Text("-- %d -->", display_now_point);
                    ImGui::SameLine();
                    ImGui::Text(point_name[display_route[display_now_point]].c_str());
                    ImGui::SameLine();
                    ImGui::Text("-- %d -->", len - display_now_point);
                    ImGui::SameLine();
                    ImGui::Text(point_name[display_route[len]].c_str());
                    ImGui::Text("剩余站数: %d ", len - display_now_point);
                }
                ImGui::End();
            }

            // 5. Change to another city

            {
                const int space = (int)ImGui::GetTextLineHeightWithSpacing();
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize;
                ImGui::SetNextWindowSize(ImVec2((float)(16.0*space), (float)(16.0*space)));
                ImGui::Begin("op&ed select", (bool*)0, window_flags);

                //static int all_line = 0, all_point = 0; // Here we store our selection data as an index.

                if (ImGui::Button("切换城市", ImVec2(-FLT_MIN, (float)space * 2))) {
                    city_map_flag = (city_map_flag + 1) % 2;// 2为城市总数
                    // 重新初始化
                    init();
                    init_gui();
                    input_map();
                    input2();
                    is_route_displaying = false, is_flick = false;
                    if(city_map_flag==0)
                        cout << "toggle to beijing" << endl;
                    else
                        cout << "toggle to wuhan" << endl;

                }

                
                ImGui::End();
            }



                   
                    
            // Rendering
            ImGui::Render();

            FrameContext* frameCtx = WaitForNextFrameResources();
            UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
            frameCtx->CommandAllocator->Reset();

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = g_mainRenderTargetResource[backBufferIdx];
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            g_pd3dCommandList->Reset(frameCtx->CommandAllocator, nullptr);
            g_pd3dCommandList->ResourceBarrier(1, &barrier);

            // Render Dear ImGui graphics
            const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
            g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, nullptr);
            g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
            g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            g_pd3dCommandList->ResourceBarrier(1, &barrier);
            g_pd3dCommandList->Close();

            g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);

            g_pSwapChain->Present(1, 0); // Present with vsync
            //g_pSwapChain->Present(0, 0); // Present without vsync

            UINT64 fenceValue = g_fenceLastSignaledValue + 1;
            g_pd3dCommandQueue->Signal(g_fence, fenceValue);
            g_fenceLastSignaledValue = fenceValue;
            frameCtx->FenceValue = fenceValue;
        }

        WaitForLastSubmittedFrame();

        // Cleanup
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D();
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        
        

    }



    system("pause");
    return 0;
}
