#include "pch.h"
#include "InputManager.h"
#include "D2DTransform.h"
#include "SolarSystemRenderer.h"
#include "TransformPracticeScene.h"

using namespace std;

using TestRenderer = myspace::D2DRenderer;
using Vec2 = MYHelper::Vector2F;

constexpr int SOLAR_SYSTEM = 9;
constexpr int SOLAR_PLANETS = 8;
constexpr int SOLAR_SATLELITE = 13;

class CelestialObj {
private:
    CelestialObj() = delete; // 자체 생성 방지
    CelestialObj(const CelestialObj&) = delete; // 대입 생성자 방지
public:
    CelestialObj(ComPtr<ID2D1Bitmap1>& bitmap) {
        m_BitmapPtr = bitmap;

        ++s_id; // ++오브젝트 카운트
        m_name += to_wstring(s_id); // ID를 이름에 추가

        m_renderTM = MYTM::MakeRenderMatrix(true); // 렌더 행렬 만들기

        D2D1_SIZE_F size = { m_rect.right - m_rect.left, m_rect.bottom - m_rect.top };

        //m_transform.SetPivotPreset(D2DTM::PivotPreset::TopLeft, size);
        //m_transform.SetPivotPreset(D2DTM::PivotPreset::BottomRight, size);
        m_transform.SetPivotPreset(D2DTM::PivotPreset::Center, size);
    }

    ~CelestialObj() = default;

    void Update(float deltaTime)
    {
        if (m_isSelfRotation) {
            m_transform.Rotate(deltaTime * m_selfRotation); // 자기 회전 (시간 * 회전량)
        }
    }
        
    void Draw(TestRenderer& testRender, D2D1::Matrix3x2F viewTM) {

        if (m_isInvisible) return;
        //static D2D1_RECT_F s_rect = D2D1::RectF(0.f, 0.f, 100.f, 100.f);
        static D2D1_RECT_F s_rect = D2D1::RectF(0.f, 0.f, 100.f, 100.f);

        D2D1::Matrix3x2F worldTM = m_transform.GetWorldMatrix(); // 월드 좌표계

        D2D1::Matrix3x2F finalTM = m_renderTM * worldTM * viewTM; // 최종 좌표계

        D2D1::ColorF celColor = D2D1::ColorF::LightGray;

        if (m_isLeader) celColor = D2D1::ColorF::Red; // 부모일 경우 RED
        else if (m_isSelected) celColor = D2D1::ColorF::HotPink; // 선택의 경우 HotPink

        testRender.SetTransform(finalTM); // 트랜스폼 세터. Matrix3x2F
        testRender.DrawRectangle(s_rect.left, s_rect.top, s_rect.right, s_rect.bottom, celColor); // 그리기

        D2D1_RECT_F dest = D2D1::RectF(s_rect.left, s_rect.top, s_rect.right, s_rect.bottom); // 비트맵 크기
        testRender.DrawBitmap(m_BitmapPtr.Get(), dest); // 비트맵,dest(크기)를 가져와 그리기
        testRender.DrawMessage(m_name.c_str(), s_rect.left, s_rect.top, 200, 50, D2D1::ColorF::Black);
    }

    void SetPosition(const Vec2& pos) { m_transform.SetPosition(pos); } // Transform Pos 위치 이동
    void SetCenterPosition() { m_transform.SetPosition({ -m_rect.right / 2.0f, -m_rect.bottom / 2.0f }); }
    void SetInvisible(bool invisible) { m_isInvisible = invisible; }
    void Move(const Vec2& offset) { m_transform.Translate(offset); } // Transform 보간 이동
    void Rotate(float angle) { m_transform.Rotate(angle); } // Transform 회전
    void SetSelfRotate(float angle) { m_selfRotation = angle; }
    void ToggleSelected() { m_isSelected = !m_isSelected; } // 선택 토글
    bool IsSelected() const { return m_isSelected; } // 선택 여부 리턴
    void ToggleSelfRotation() { m_isSelfRotation = !m_isSelfRotation; } // 자기 회전 토글
    bool IsHitTest(D2D1_POINT_2F worldPoint, D2D1::Matrix3x2F viewTM) { // 클릭 테스트
        D2D1::Matrix3x2F worldTM = m_transform.GetWorldMatrix(); // 월드 행렬 ( 로컬 -> 월드 )

        // 전체 변환 행렬 (로컬 → 월드 → 뷰)
        D2D1::Matrix3x2F finalTM = m_renderTM * worldTM * viewTM; // 로컬 → 화면 좌표 변환
        
        // 역행렬로 전환 (뷰 → 월드 → 로컬)
        finalTM.Invert(); // 화면 → 로컬 좌표 변환

        // 2) 로컬 좌표로 포인트 변환
        //( 뷰 좌표계 상의 점 -> 로컬 좌표 )
        D2D1_POINT_2F localPt = finalTM.TransformPoint(worldPoint); // 역연산한 전체 행렬(로컬)에서의 월드 좌표를 가져옴

        // 3) 로컬 사각형 정의
        // (0,0) ~ (width, height) 범위에 있다면 히트!
        // m_rect = D2D1::RectF(0.f, 0.f, 100.f, 100.f);

        std::cout << "BoxObject::IsHitTest: localPt = ("
            << localPt.x << ", " << localPt.y << ")" << std::endl;

        std::cout << "BoxObject::IsHitTest: m_rect = ("
            << m_rect.left << ", " << m_rect.top << ", "
            << m_rect.right << ", " << m_rect.bottom << ")" << std::endl;

        // 4) 로컬 공간에서 검사
        return MYTM::IsPointInRect(localPt, m_rect); // 사각형(m_rect) 안의 범위에 localPt가 있다면 히트 (오른쪽 아래로 100 * 100)
    }

    D2DTM::Transform* GetTransform() { return &m_transform; } // Transform Getter()
    void SetParent(CelestialObj* parent) { // 부모 지정
        // assert : 반드시 true여야 하는 조건식. false면 프로그램 중단 및 에러코드 출력.
        // Release모드에서 assert는 제거됨.
        assert(parent != nullptr); 
        if (nullptr != m_transform.GetParent()) {
            // 이미 부모가 있다면 부모 관계 해제
            m_transform.DetachFromParent();
        }
        m_transform.SetParent(parent->GetTransform());
    }

    void DetachFromParent() { m_transform.DetachFromParent(); }
    void SetLeader(bool isLeader) { m_isLeader = isLeader; }

private:
    D2DTM::Transform m_transform; // 트랜스폼

    MAT3X2F m_renderTM; // 렌더링 변환 행렬
   
    D2D1_RECT_F m_rect = D2D1::RectF(0.f, 0.f, 100.f, 100.f); // 기본 비트맵 그림 위치

    // L : 유니코드 문자열 리터럴접두사 
    // wchar_t : 유니코드 및 다중 언어 지원
    wstring m_name = L""; 

    bool m_isSelected = false; // 선택 여부
    bool m_isLeader = false; // 리더 박스 여부
    bool m_isSelfRotation = false; // 자전 여부
    bool m_isInvisible = false; // 비트맵 그리기 여부

    float m_selfRotation = 36.f;

    ComPtr<ID2D1Bitmap1> m_BitmapPtr; // 비트맵 주소
    
    static int s_id; // static 멤버 변수로 ID 관리 (예제용)
};

int CelestialObj::s_id = 0; // static 멤버 변수 초기화

// HierarchicalTransformTestScene : 유니티 좌표계 카메라, F1 : 고양이 생성, F2: 계층 구조
// TransformPracticeScene : 유니티 좌표계 카메라, 태양계 기본 생성

TransformPracticeScene::~TransformPracticeScene(){
    for (auto& obj : m_CelObjects) { delete obj; }
}

void TransformPracticeScene::SetUp(HWND hWnd)
{
    constexpr int defaultGameObjCount = 100;

    m_CelObjects.reserve(defaultGameObjCount);

    m_hWnd = hWnd;

    SetWindowText(m_hWnd, 
    L"가상의 태양계를 만들어 주세요. 물리 법칙은 무시 합니다. ^^;;");

    std::cout << "태양은 자전을 해야 합니다." << std::endl;
    std::cout << "행성들은 자전을 하며 동시에 태양의 자전에 영향을 받아 공전하는 것처럼 보입니다."<< std::endl;
    std::cout << "달은 자전을 하면서 동시에 지구의 자전에 영향을 받아 공전하는 것처럼 보입니다." << std::endl;
    std::cout << "회전 속도는 자유롭게 설정하세요." << std::endl;

    BitMapSetUp(); // 비트맵 셋업

    RECT rc;

    D2D1_POINT_2F center{};
    if (::GetClientRect(hWnd, &rc))
    {
        float w = static_cast<float>(rc.right - rc.left);
        float h = static_cast<float>(rc.bottom - rc.top);

        center = D2D1_POINT_2F{ w / 2,h / 2 };
        m_UnityCamera.SetScreenSize(w, h);
    }
    // 태양
    AddCelObjects(center);
    CelestialObj* sun = m_CelObjects.back();
    sun->GetTransform()->SetScale(Vec2(3.f, 3.f));
    sun->SetCenterPosition();
    sun->SetLeader(true);
    // 태양은 자전 없음

    float dis = 120.f;
    float n = 90.f;

    // 수성 pivot (공전)
    AddCelObjects(center);
    CelestialObj* mercury_pivot = m_CelObjects.back();
    mercury_pivot->SetInvisible(true);
    mercury_pivot->SetCenterPosition();
    mercury_pivot->SetParent(sun);
    mercury_pivot->SetSelfRotate(30.f);
    mercury_pivot->ToggleSelfRotation();

    // 수성 (자전)
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Mercury = m_CelObjects.back();
    Mercury->SetParent(mercury_pivot);
    Mercury->GetTransform()->SetScale(Vec2(0.5f, 0.5f));
    Mercury->SetCenterPosition();
    Mercury->SetPosition(Vec2(dis, dis));
    Mercury->SetSelfRotate(24.f);
    Mercury->ToggleSelfRotation();
    dis += n;

    // 금성 pivot (공전)
    AddCelObjects(center);
    CelestialObj* venus_pivot = m_CelObjects.back();
    venus_pivot->SetInvisible(true);
    venus_pivot->SetCenterPosition();
    venus_pivot->SetParent(sun);
    venus_pivot->SetSelfRotate(20.f);
    venus_pivot->ToggleSelfRotation();

    // 금성 (자전)
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Venus = m_CelObjects.back();
    Venus->SetParent(venus_pivot);
    Venus->GetTransform()->SetScale(Vec2(0.65f, 0.65f));
    Venus->SetCenterPosition();
    Venus->SetPosition(Vec2(dis, dis));
    Venus->SetSelfRotate(16.f);
    Venus->ToggleSelfRotation();
    dis += n;

    // 지구 pivot (공전)
    AddCelObjects(center);
    CelestialObj* earth_pivot = m_CelObjects.back();
    earth_pivot->SetInvisible(true);
    earth_pivot->SetCenterPosition();
    earth_pivot->SetParent(sun);
    earth_pivot->SetSelfRotate(16.f);
    earth_pivot->ToggleSelfRotation();

    // 지구 (자전)
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Earth = m_CelObjects.back();
    Earth->SetParent(earth_pivot);
    Earth->GetTransform()->SetScale(Vec2(0.7f, 0.7f));
    Earth->SetCenterPosition();
    Earth->SetPosition(Vec2(dis, dis));
    Earth->SetSelfRotate(18.f);
    Earth->ToggleSelfRotation();
    dis += n;

    // 달 (자전)
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Moon = m_CelObjects.back();
    Moon->SetParent(Earth);
    Moon->GetTransform()->SetScale(Vec2(0.3f, 0.3f));
    Moon->SetCenterPosition();
    Moon->SetPosition(Vec2(30.f, 0.f));
    Moon->SetSelfRotate(24.f);
    Moon->ToggleSelfRotation();

    // 화성 pivot (공전)
    AddCelObjects(center);
    CelestialObj* mars_pivot = m_CelObjects.back();
    mars_pivot->SetInvisible(true);
    mars_pivot->SetCenterPosition();
    mars_pivot->SetParent(sun);
    mars_pivot->SetSelfRotate(12.f);
    mars_pivot->ToggleSelfRotation();

    // 화성 (자전)
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Mars = m_CelObjects.back();
    Mars->SetParent(mars_pivot);
    Mars->GetTransform()->SetScale(Vec2(0.6f, 0.6f));
    Mars->SetCenterPosition();
    Mars->SetPosition(Vec2(dis, dis));
    Mars->SetSelfRotate(15.f);
    Mars->ToggleSelfRotation();
    dis += n;

    // 목성 pivot (공전)
    AddCelObjects(center);
    CelestialObj* jupiter_pivot = m_CelObjects.back();
    jupiter_pivot->SetInvisible(true);
    jupiter_pivot->SetCenterPosition();
    jupiter_pivot->SetParent(sun);
    jupiter_pivot->SetSelfRotate(8.f);
    jupiter_pivot->ToggleSelfRotation();

    // 목성 (자전)
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Jupiter = m_CelObjects.back();
    Jupiter->SetParent(jupiter_pivot);
    Jupiter->GetTransform()->SetScale(Vec2(1.5f, 1.5f));
    Jupiter->SetCenterPosition();
    Jupiter->SetPosition(Vec2(dis, dis));
    Jupiter->SetSelfRotate(12.f);
    Jupiter->ToggleSelfRotation();
    dis += n;

    // 토성 pivot (공전)
    AddCelObjects(center);
    CelestialObj* saturn_pivot = m_CelObjects.back();
    saturn_pivot->SetInvisible(true);
    saturn_pivot->SetCenterPosition();
    saturn_pivot->SetParent(sun);
    saturn_pivot->SetSelfRotate(7.f);
    saturn_pivot->ToggleSelfRotation();

    // 토성 (자전)
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Saturn = m_CelObjects.back();
    Saturn->SetParent(saturn_pivot);
    Saturn->GetTransform()->SetScale(Vec2(1.3f, 1.3f));
    Saturn->SetCenterPosition();
    Saturn->SetPosition(Vec2(dis, dis));
    Saturn->SetSelfRotate(10.f);
    Saturn->ToggleSelfRotation();
    dis += n;

    // 천왕성 pivot (공전)
    AddCelObjects(center);
    CelestialObj* uranus_pivot = m_CelObjects.back();
    uranus_pivot->SetInvisible(true);
    uranus_pivot->SetCenterPosition();
    uranus_pivot->SetParent(sun);
    uranus_pivot->SetSelfRotate(6.f);
    uranus_pivot->ToggleSelfRotation();

    // 천왕성 (자전)
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Uranus = m_CelObjects.back();
    Uranus->SetParent(uranus_pivot);
    Uranus->GetTransform()->SetScale(Vec2(1.0f, 1.0f));
    Uranus->SetCenterPosition();
    Uranus->SetPosition(Vec2(dis, dis));
    Uranus->SetSelfRotate(8.f);
    Uranus->ToggleSelfRotation();
    dis += n;

    // 해왕성 pivot (공전)
    AddCelObjects(center);
    CelestialObj* neptune_pivot = m_CelObjects.back();
    neptune_pivot->SetInvisible(true);
    neptune_pivot->SetCenterPosition();
    neptune_pivot->SetParent(sun);
    neptune_pivot->SetSelfRotate(5.f);
    neptune_pivot->ToggleSelfRotation();

    // 해왕성 (자전)
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Neptune = m_CelObjects.back();
    Neptune->SetParent(neptune_pivot);
    Neptune->GetTransform()->SetScale(Vec2(0.9f, 0.9f));
    Neptune->SetCenterPosition();
    Neptune->SetPosition(Vec2(dis, dis));
    Neptune->SetSelfRotate(7.f);
    Neptune->ToggleSelfRotation();
    dis += n;

}

void TransformPracticeScene::Tick(float deltaTime)
{
    // 입력 이벤트 처리
    ProcessMouseEvents();
    ProcessKeyboardEvents();


    for (auto& obj : m_CelObjects) {
        obj->Update(deltaTime);
    }
    // 카메라 업데이트

    MAT3X2F cameraTM = m_UnityCamera.GetViewMatrix();

    MAT3X2F renderTM = MYTM::MakeRenderMatrix(true); // 카메라 위치 렌더링 매트릭스

    MAT3X2F finalTM = renderTM * cameraTM; // 카메라 -> 렌더

    // 렌더링

    static myspace::D2DRenderer& globalRenderer = SolarSystemRenderer::Instance();

    wchar_t buffer[128] = L"";
    MYTM::MakeMatrixToString(cameraTM, buffer, 128);

    globalRenderer.RenderBegin(); // 렌더 생성

    globalRenderer.SetTransform(finalTM);

    // 카메라 위치 표시
    //globalRenderer.DrawRectangle(-10.f, 10.f, 10.f, -10.f, D2D1::ColorF::Red);
    //globalRenderer.DrawCircle(0.f, 0.f, 5.f, D2D1::ColorF::Red);
    //globalRenderer.DrawMessage(buffer, 10.f, 10.f, 100.f, 100.f, D2D1::ColorF::Black);

    for (auto& obj : m_CelObjects) {
        obj->Draw(globalRenderer, cameraTM);
    }

    globalRenderer.RenderEnd();

    // 휠 입력 초기화
    InputManager::Instance().SetMouseWheelDelta(0);
}
void TransformPracticeScene::BitMapSetUp()
{
    std::wstring celBitmap[] = {
        L"../Resource/cat.png", // Sun
        L"../Resource/cat.png", // Mercury
        L"../Resource/cat.png", // Venus
        L"../Resource/cat.png", // Earth
        L"../Resource/cat.png", // Mars
        L"../Resource/cat.png", // Jupiter
        L"../Resource/cat.png", // Saturn
        L"../Resource/cat.png", // Uranus
        L"../Resource/cat.png"  // Neptune
    };

    std::wstring subCelBitmap[] = {
        L"../Resource/cat.png", // Moon
        L"../Resource/cat.png", // Phobos
        L"../Resource/cat.png", // Deimos
        L"../Resource/cat.png", // Io
        L"../Resource/cat.png", // Europa
        L"../Resource/cat.png", // Ganymede
        L"../Resource/cat.png", // Callisto
        L"../Resource/cat.png", // Titan
        L"../Resource/cat.png", // Enceladus
        L"../Resource/cat.png", // Miranda
        L"../Resource/cat.png", // Titania
        L"../Resource/cat.png", // Oberon
        L"../Resource/cat.png"  // Triton
    };

    SolarSystemRenderer::Instance().CreateBitmapFromFile(L"../Resource/cat.png", *m_BitmapPtr.GetAddressOf()); // 기본값 고양이

    for (int i = 0; i < 9; ++i)
    {
        SolarSystemRenderer::Instance().CreateBitmapFromFile(celBitmap[i].c_str(), *m_CelBitmapPtr[i].GetAddressOf()); // 태양 및 행성
    }

    for (int i = 0; i < 13; ++i)
    {
        SolarSystemRenderer::Instance().CreateBitmapFromFile(subCelBitmap[i].c_str(), *m_SatBitmapPtr[i].GetAddressOf()); // 위성
    }

}

void TransformPracticeScene::OnResize(int width, int height)
{ 
    // 윈도우 크기 변경 시 카메라의 화면 크기를 업데이트
    m_UnityCamera.SetScreenSize(width, height);
}

void TransformPracticeScene::ProcessKeyboardEvents()
{
    /*
    // 클리어
    if (InputManager::Instance().GetKeyPressed(VK_F1))
    {
        ClearBoxObjects();
    }

    if (InputManager::Instance().GetKeyPressed(VK_F2))
    {
        SetBoxSelfRotation();
    }


    // 카메라 이동 처리, 
    static const std::vector<std::pair<int, Vec2>> kCameraMoves = {
      { VK_RIGHT, {  1.f,  0.f } },
      { VK_LEFT,  { -1.f,  0.f } },
      { VK_UP,    {  0.f,  1.f } },
      { VK_DOWN,  {  0.f, -1.f } },
    };

    // C++17부터는 structured binding을 사용하여 더 간결하게 표현할 수 있습니다.
    for (auto& [vk, dir] : kCameraMoves)
    {
        if (InputManager::Instance().GetKeyDown(vk))
        {
            m_UnityCamera.Move(dir.x, dir.y);
        }
    }

    // 첫번째 선택된 박스를 이동
    static const std::vector<std::pair<int, Vec2>> kBoxMoves = {
      { 'D', {  1.f,  0.f } }, // D키로 오른쪽 이동
      { 'A', { -1.f,  0.f } }, // A키로 왼쪽 이동
      { 'W', {  0.f,  1.f } }, // W키로 위로 이동
      { 'S', {  0.f, -1.f } }, // S키로 아래로 이동

    };

    for (auto& [vk, dir] : kBoxMoves)
    {
        if (InputManager::Instance().GetKeyDown(vk))
        {
            if (m_SelectedCelObjects.size())
                m_SelectedCelObjects.front()->Move(dir);
        }
    }

    // 첫번째 선택된 박스를 회전
    if (InputManager::Instance().GetKeyDown(VK_SPACE) && !m_SelectedCelObjects.empty())
    {
        m_SelectedCelObjects.front()->Rotate(1.f); // 각도 단위로 회전, // 트랜스폼 값을 바꿈
    }
    */
}

void TransformPracticeScene::OnMouseLButtonDown(D2D1_POINT_2F pos)
{
    std::cout << "Left button up at: " << pos.x << ", " << pos.y << std::endl;
    //AddCelObjects(pos);
}

void TransformPracticeScene::OnMouseRButtonDown(D2D1_POINT_2F pos)
{
    std::cout << "Right button down at: " << pos.x << ", " << pos.y << std::endl;
    //AddCelObjects(pos);
}
void TransformPracticeScene::OnMouseWheel(int delta, D2D1_POINT_2F pos)
{
    /*std::cout << "mouse wheel (" << pos.x << ", " << pos.y << ") to ("
        << delta << ")" << std::endl;*/
    float zoom = m_UnityCamera.GetZoom();

    constexpr float zoomSensitivity = 0.001f;
    zoom += delta * zoomSensitivity;

    zoom = std::clamp(zoom, 0.1f, 100.f); // 너무 작거나 크게 안 가도록 제한
    m_UnityCamera.SetZoom(zoom);
}   

void TransformPracticeScene::AddCelObjects(D2D1_POINT_2F point) { // 스크린 좌표 획득

    static int solarCount = 0;
    MAT3X2F cameraTM = m_UnityCamera.GetViewMatrix(); // 카메라 행렬 (뷰)를 획득
    cameraTM.Invert(); // 카메라 행렬 역연산

    D2D1_POINT_2F worldPt = cameraTM.TransformPoint(point); // 월드 포인터를 받아옴

    CelestialObj* pNewObj = nullptr;

    pNewObj = new CelestialObj(m_BitmapPtr); // 오브젝트 생성

    if (m_CelObjects.empty()) { pNewObj->SetLeader(true); } // 첫 생성 시 리더 설정

    pNewObj->SetPosition(Vec2(worldPt.x, worldPt.y)); // 월드 포지션으로 세팅

    m_CelObjects.push_back(pNewObj); // 생성된 Obj 배열에 추가
}
void TransformPracticeScene::AddCelObjects(D2D1_POINT_2F point, E_CelestialObj celObj) { // 스크린 좌표 획득

    static int solarCount = 0;
    MAT3X2F cameraTM = m_UnityCamera.GetViewMatrix(); // 카메라 행렬 (뷰)를 획득
    cameraTM.Invert(); // 카메라 행렬 역연산

    D2D1_POINT_2F worldPt = cameraTM.TransformPoint(point); // 월드 포인터를 받아옴

    CelestialObj* pNewObj = nullptr;

    // 오브젝트 생성
    switch (celObj)
    {
    case E_CelestialObj::Sun:        pNewObj = new CelestialObj(m_CelBitmapPtr[0]); break;
    case E_CelestialObj::Mercury:    pNewObj = new CelestialObj(m_CelBitmapPtr[1]); break;
    case E_CelestialObj::Venus:      pNewObj = new CelestialObj(m_CelBitmapPtr[2]); break;
    case E_CelestialObj::Earth:      pNewObj = new CelestialObj(m_CelBitmapPtr[3]); break;
    case E_CelestialObj::Mars:       pNewObj = new CelestialObj(m_CelBitmapPtr[4]); break;
    case E_CelestialObj::Jupiter:    pNewObj = new CelestialObj(m_CelBitmapPtr[5]); break;
    case E_CelestialObj::Saturn:     pNewObj = new CelestialObj(m_CelBitmapPtr[6]); break;
    case E_CelestialObj::Uranus:     pNewObj = new CelestialObj(m_CelBitmapPtr[7]); break;
    case E_CelestialObj::Neptune:    pNewObj = new CelestialObj(m_CelBitmapPtr[8]); break;
    default:
        pNewObj = nullptr; break;
    }
     
    if (m_CelObjects.empty()) { pNewObj->SetLeader(true); } // 첫 생성 시 리더 설정

    pNewObj->SetPosition(Vec2(worldPt.x, worldPt.y)); // 월드 포지션으로 세팅

    m_CelObjects.push_back(pNewObj); // 생성된 Obj 배열에 추가
}
void TransformPracticeScene::AddCelObjects(D2D1_POINT_2F point, E_Satellite) { // 스크린 좌표 획득

    static int solarCount = 0;
    MAT3X2F cameraTM = m_UnityCamera.GetViewMatrix(); // 카메라 행렬 (뷰)를 획득
    cameraTM.Invert(); // 카메라 행렬 역연산

    D2D1_POINT_2F worldPt = cameraTM.TransformPoint(point); // 월드 포인터를 받아옴

    CelestialObj* pNewObj = nullptr;
    
    pNewObj = new CelestialObj(m_BitmapPtr); // 오브젝트 생성

    if (m_CelObjects.empty()) { pNewObj->SetLeader(true); } // 첫 생성 시 리더 설정

    pNewObj->SetPosition(Vec2(worldPt.x, worldPt.y)); // 월드 포지션으로 세팅

    m_CelObjects.push_back(pNewObj); // 생성된 Obj 배열에 추가
}


void TransformPracticeScene::ClearCelObjects()
{
    for (auto& box : m_CelObjects)
    {
        delete box;
    }

    m_CelObjects.clear();

    m_SelectedCelObjects.clear();
}

void TransformPracticeScene::SelectCelObject(D2D1_POINT_2F point)
{
    /*MAT3X2F cameraTM = m_UnityCamera.GetViewMatrix();

    std::list<BoxObject*> oldSelectedList = std::move(m_SelectedBoxObjects); // lvalue 타입을 rvalue 타입으로 타입캐스팅

    std::cout << "size of oldSelectedList: " << oldSelectedList.size() << std::endl;


    for (auto& box : m_BoxObjects)
    {
        if (box->IsHitTest(point, cameraTM))
        {
            box->ToggleSelected();

            if (box->IsSelected()) m_SelectedBoxObjects.push_back(box); // 새로 선택된 박스 추가
        }
    }

    // 기존 계층 관계 해제하고 선택된 박스 목록 재구성
    for (auto it = oldSelectedList.crbegin(); it != oldSelectedList.crend(); ++it)
    {
        (*it)->DetachFromParent();

        if ((*it)->IsSelected())
        {
            m_SelectedBoxObjects.push_front(*it);
        }
        else
        {
            (*it)->SetLeader(false);
        }

    }

    std::cout << "size of m_SelectedBoxObjects: " << m_SelectedBoxObjects.size() << std::endl;

    // 계층 구조 업데이트
    UpdateRelationship();
    */
}

void TransformPracticeScene::SetCelSelfRotation() {
    for (auto obj : m_CelObjects)
    {
        if (false == obj->IsSelected()) obj->ToggleSelfRotation();
    }
}

void TransformPracticeScene::UpdateRelationship()
{
    auto it = m_SelectedCelObjects.begin();

    if (it == m_SelectedCelObjects.end()) return; // 선택된 박스가 없으면 종료

    (*it)->SetLeader(true); // 첫번째 박스를 리더로 설정

    if (m_SelectedCelObjects.size() < 2) return; // 선택된 박스가 2개 미만이면 종료

    while (it != m_SelectedCelObjects.end() && std::next(it) != m_SelectedCelObjects.end())
    {
        CelestialObj* parent = *it;
        CelestialObj* child = *(std::next(it));

        child->SetParent(parent);

        it++;

        std::cout << "부모: " << parent->GetTransform()->GetPosition().x << ", "
            << parent->GetTransform()->GetPosition().y
            << " 자식: " << child->GetTransform()->GetPosition().x << ", "
            << child->GetTransform()->GetPosition().y << std::endl;
    }
}
