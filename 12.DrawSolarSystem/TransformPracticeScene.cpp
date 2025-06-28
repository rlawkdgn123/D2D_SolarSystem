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
    CelestialObj() = delete; // ��ü ���� ����
    CelestialObj(const CelestialObj&) = delete; // ���� ������ ����
public:
    CelestialObj(ComPtr<ID2D1Bitmap1>& bitmap) {
        m_BitmapPtr = bitmap;

        ++s_id; // ++������Ʈ ī��Ʈ
        m_name += to_wstring(s_id); // ID�� �̸��� �߰�

        m_renderTM = MYTM::MakeRenderMatrix(true); // ���� ��� �����

        D2D1_SIZE_F size = { m_rect.right - m_rect.left, m_rect.bottom - m_rect.top };

        //m_transform.SetPivotPreset(D2DTM::PivotPreset::TopLeft, size);
        //m_transform.SetPivotPreset(D2DTM::PivotPreset::BottomRight, size);
        m_transform.SetPivotPreset(D2DTM::PivotPreset::Center, size);
    }

    ~CelestialObj() = default;

    void Update(float deltaTime)
    {
        if (m_isSelfRotation) {
            m_transform.Rotate(deltaTime * 36.f); // �ڱ� ȸ�� (�ð� * ȸ����)
        }
    }
        
    void Draw(TestRenderer& testRender, D2D1::Matrix3x2F viewTM) {
        //static D2D1_RECT_F s_rect = D2D1::RectF(0.f, 0.f, 100.f, 100.f);
        static D2D1_RECT_F s_rect = D2D1::RectF(0.f, 0.f, 100.f, 100.f);

        D2D1::Matrix3x2F worldTM = m_transform.GetWorldMatrix(); // ���� ��ǥ��

        D2D1::Matrix3x2F finalTM = m_renderTM * worldTM * viewTM; // ���� ��ǥ��

        D2D1::ColorF celColor = D2D1::ColorF::LightGray;

        if (m_isLeader) celColor = D2D1::ColorF::Red; // �θ��� ��� RED
        else if (m_isSelected) celColor = D2D1::ColorF::HotPink; // ������ ��� HotPink

        testRender.SetTransform(finalTM); // Ʈ������ ����. Matrix3x2F
        testRender.DrawRectangle(s_rect.left, s_rect.top, s_rect.right, s_rect.bottom, celColor); // �׸���

        D2D1_RECT_F dest = D2D1::RectF(s_rect.left, s_rect.top, s_rect.right, s_rect.bottom); // ��Ʈ�� ũ��
        testRender.DrawBitmap(m_BitmapPtr.Get(), dest); // ��Ʈ��,dest(ũ��)�� ������ �׸���
        testRender.DrawMessage(m_name.c_str(), s_rect.left, s_rect.top, 200, 50, D2D1::ColorF::Black);
    }

    void SetPosition(const Vec2& pos) { m_transform.SetPosition(pos); } // Transform Pos ��ġ �̵�
    void Move(const Vec2& offset) { m_transform.Translate(offset); } // Transform ���� �̵�
    void Rotate(float angle) { m_transform.Rotate(angle); } // Transform ȸ��
    void ToggleSelected() { m_isSelected = !m_isSelected; } // ���� ���
    bool IsSelected() const { return m_isSelected; } // ���� ���� ����
    void ToggleSelfRotation() { m_isSelfRotation = !m_isSelfRotation; } // �ڱ� ȸ�� ���
    bool IsHitTest(D2D1_POINT_2F worldPoint, D2D1::Matrix3x2F viewTM) { // Ŭ�� �׽�Ʈ
        D2D1::Matrix3x2F worldTM = m_transform.GetWorldMatrix(); // ���� ��� ( ���� -> ���� )

        // ��ü ��ȯ ��� (���� �� ���� �� ��)
        D2D1::Matrix3x2F finalTM = m_renderTM * worldTM * viewTM; // ���� �� ȭ�� ��ǥ ��ȯ
        
        // ����ķ� ��ȯ (�� �� ���� �� ����)
        finalTM.Invert(); // ȭ�� �� ���� ��ǥ ��ȯ

        // 2) ���� ��ǥ�� ����Ʈ ��ȯ
        //( �� ��ǥ�� ���� �� -> ���� ��ǥ )
        D2D1_POINT_2F localPt = finalTM.TransformPoint(worldPoint); // �������� ��ü ���(����)������ ���� ��ǥ�� ������

        // 3) ���� �簢�� ����
        // (0,0) ~ (width, height) ������ �ִٸ� ��Ʈ!
        // m_rect = D2D1::RectF(0.f, 0.f, 100.f, 100.f);

        std::cout << "BoxObject::IsHitTest: localPt = ("
            << localPt.x << ", " << localPt.y << ")" << std::endl;

        std::cout << "BoxObject::IsHitTest: m_rect = ("
            << m_rect.left << ", " << m_rect.top << ", "
            << m_rect.right << ", " << m_rect.bottom << ")" << std::endl;

        // 4) ���� �������� �˻�
        return MYTM::IsPointInRect(localPt, m_rect); // �簢��(m_rect) ���� ������ localPt�� �ִٸ� ��Ʈ (������ �Ʒ��� 100 * 100)
    }

    D2DTM::Transform* GetTransform() { return &m_transform; } // Transform Getter()
    void SetParent(CelestialObj* parent) { // �θ� ����
        // assert : �ݵ�� true���� �ϴ� ���ǽ�. false�� ���α׷� �ߴ� �� �����ڵ� ���.
        // Release��忡�� assert�� ���ŵ�.
        assert(parent != nullptr); 
        if (nullptr != m_transform.GetParent()) {
            // �̹� �θ� �ִٸ� �θ� ���� ����
            m_transform.DetachFromParent();
        }
        m_transform.SetParent(parent->GetTransform());
    }

    void DetachFromParent() { m_transform.DetachFromParent(); }
    void SetLeader(bool isLeader) { m_isLeader = isLeader; }

private:
    D2DTM::Transform m_transform; // Ʈ������

    MAT3X2F m_renderTM; // ������ ��ȯ ���
   
    D2D1_RECT_F m_rect = D2D1::RectF(0.f, 0.f, 100.f, 100.f); // �⺻ ��Ʈ�� �׸� ��ġ

    // L : �����ڵ� ���ڿ� ���ͷ����λ� 
    // wchar_t : �����ڵ� �� ���� ��� ����
    wstring m_name = L""; 

    bool m_isSelected = false; // ���� ����
    bool m_isLeader = false; // ���� �ڽ� ����
    bool m_isSelfRotation = false; // ���� ����

    ComPtr<ID2D1Bitmap1> m_BitmapPtr; // ��Ʈ�� �ּ�
    
    static int s_id; // static ��� ������ ID ���� (������)
};

int CelestialObj::s_id = 0; // static ��� ���� �ʱ�ȭ

// HierarchicalTransformTestScene : ����Ƽ ��ǥ�� ī�޶�, F1 : ����� ����, F2: ���� ����
// TransformPracticeScene : ����Ƽ ��ǥ�� ī�޶�, �¾�� �⺻ ����

TransformPracticeScene::~TransformPracticeScene(){
    for (auto& obj : m_CelObjects) { delete obj; }
}

void TransformPracticeScene::SetUp(HWND hWnd)
{
    constexpr int defaultGameObjCount = 100;

    m_CelObjects.reserve(defaultGameObjCount);

    m_hWnd = hWnd;

    SetWindowText(m_hWnd, 
    L"������ �¾�踦 ����� �ּ���. ���� ��Ģ�� ���� �մϴ�. ^^;;");

    std::cout << "�¾��� ������ �ؾ� �մϴ�." << std::endl;
    std::cout << "�༺���� ������ �ϸ� ���ÿ� �¿��� ������ ������ �޾� �����ϴ� ��ó�� ���Դϴ�."<< std::endl;
    std::cout << "���� ������ �ϸ鼭 ���ÿ� ������ ������ ������ �޾� �����ϴ� ��ó�� ���Դϴ�." << std::endl;
    std::cout << "ȸ�� �ӵ��� �����Ӱ� �����ϼ���." << std::endl;

    BitMapSetUp(); // ��Ʈ�� �¾�

    RECT rc;

    D2D1_POINT_2F center{};
    if (::GetClientRect(hWnd, &rc))
    {
        float w = static_cast<float>(rc.right - rc.left);
        float h = static_cast<float>(rc.bottom - rc.top);

        center = D2D1_POINT_2F{ w / 2,h / 2 };
        m_UnityCamera.SetScreenSize(w, h);
    }


    float dis = 120.f; // ���� �ʱ� �Ÿ�
    float n = 60.f;    // �⺻ ������ (����ġ�� ���� ��)

    // �¾�
    AddCelObjects(center);
    CelestialObj* sun = m_CelObjects.back();
    sun->GetTransform()->SetScale(Vec2(6.f, 6.f)); // ���� ŭ
    sun->SetPosition(Vec2(-60, -60));
    sun->SetLeader(true);
    sun->ToggleSelfRotation();

    // ����
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Mercury = m_CelObjects.back();
    Mercury->SetParent(sun);
    Mercury->GetTransform()->SetScale(Vec2(0.5f, 0.5f)); // 0.38 �� ����
    Mercury->SetPosition(Vec2(-50 + dis, -50 + dis));
    Mercury->ToggleSelfRotation();
    dis += n * 0.4f;

    // �ݼ�
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Venus = m_CelObjects.back();
    Venus->SetParent(sun);
    Venus->GetTransform()->SetScale(Vec2(0.95f, 0.95f));
    Venus->SetPosition(Vec2(-50 + dis, -50 + dis));
    Venus->ToggleSelfRotation();
    dis += n * 0.7f;

    // ����
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Earth = m_CelObjects.back();
    Earth->SetParent(sun);
    Earth->GetTransform()->SetScale(Vec2(1.0f, 1.0f));
    Earth->SetPosition(Vec2(-50 + dis, -50 + dis));
    Earth->ToggleSelfRotation();
    dis += n * 1.0f;

    // ��
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Moon = m_CelObjects.back();
    Moon->SetParent(Earth);
    Moon->GetTransform()->SetScale(Vec2(0.27f, 0.27f)); // ���� ��� �� ũ��
    Moon->SetPosition(Vec2(-30, -30)); // ���� ���� ������
    Moon->ToggleSelfRotation();

    // ȭ��
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Mars = m_CelObjects.back();
    Mars->SetParent(sun);
    Mars->GetTransform()->SetScale(Vec2(0.53f, 0.53f));
    Mars->SetPosition(Vec2(-50 + dis, -50 + dis));
    Mars->ToggleSelfRotation();
    dis += n * 1.5f;

    // ��
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Jupiter = m_CelObjects.back();
    Jupiter->SetParent(sun);
    Jupiter->GetTransform()->SetScale(Vec2(4.5f, 4.5f)); // ���� ���� 11.2���� �ʹ� ũ�� ������ ����
    Jupiter->SetPosition(Vec2(-50 + dis, -50 + dis));
    Jupiter->ToggleSelfRotation();
    dis += n * 5.2f;

    // �伺
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Saturn = m_CelObjects.back();
    Saturn->SetParent(sun);
    Saturn->GetTransform()->SetScale(Vec2(3.8f, 3.8f));
    Saturn->SetPosition(Vec2(-50 + dis, -50 + dis));
    Saturn->ToggleSelfRotation();
    dis += n * 4.0f;

    // õ�ռ�
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Uranus = m_CelObjects.back();
    Uranus->SetParent(sun);
    Uranus->GetTransform()->SetScale(Vec2(2.0f, 2.0f));
    Uranus->SetPosition(Vec2(-50 + dis, -50 + dis));
    Uranus->ToggleSelfRotation();
    dis += n * 3.5f;

    // �ؿռ�
    AddCelObjects(D2D1_POINT_2F{});
    CelestialObj* Neptune = m_CelObjects.back();
    Neptune->SetParent(sun);
    Neptune->GetTransform()->SetScale(Vec2(1.8f, 1.8f));
    Neptune->SetPosition(Vec2(-50 + dis, -50 + dis));
    Neptune->ToggleSelfRotation();
    dis += n * 3.5f;

    
}

void TransformPracticeScene::Tick(float deltaTime)
{
    // �Է� �̺�Ʈ ó��
    ProcessMouseEvents();
    ProcessKeyboardEvents();


    for (auto& obj : m_CelObjects) {
        obj->Update(deltaTime);
    }
    // ī�޶� ������Ʈ

    MAT3X2F cameraTM = m_UnityCamera.GetViewMatrix();

    MAT3X2F renderTM = MYTM::MakeRenderMatrix(true); // ī�޶� ��ġ ������ ��Ʈ����

    MAT3X2F finalTM = renderTM * cameraTM; // ī�޶� -> ����

    // ������

    static myspace::D2DRenderer& globalRenderer = SolarSystemRenderer::Instance();

    wchar_t buffer[128] = L"";
    MYTM::MakeMatrixToString(cameraTM, buffer, 128);

    globalRenderer.RenderBegin(); // ���� ����

    globalRenderer.SetTransform(finalTM);

    // ī�޶� ��ġ ǥ��
    //globalRenderer.DrawRectangle(-10.f, 10.f, 10.f, -10.f, D2D1::ColorF::Red);
    //globalRenderer.DrawCircle(0.f, 0.f, 5.f, D2D1::ColorF::Red);
    //globalRenderer.DrawMessage(buffer, 10.f, 10.f, 100.f, 100.f, D2D1::ColorF::Black);

    for (auto& obj : m_CelObjects) {
        obj->Draw(globalRenderer, cameraTM);
    }

    globalRenderer.RenderEnd();

    // �� �Է� �ʱ�ȭ
    InputManager::Instance().SetMouseWheelDelta(0);
}
void TransformPracticeScene::BitMapSetUp()
{
    string celBitmap = {};
    string subCelBitmap = {};
    SolarSystemRenderer::Instance().CreateBitmapFromFile(L"../Resource/cat.png", *m_BitmapPtr.GetAddressOf()); // �⺻�� �����

    for (int i = 0; i < SOLAR_SYSTEM; i++) { // �ϴ��� ���� ����̷�
        SolarSystemRenderer::Instance().CreateBitmapFromFile(L"../Resource/cat.png", *m_CelBitmapPtr[i].GetAddressOf());
    }
   /* for (int i = 0; i < SOLAR_SATLELITE; i++) {
        SolarSystemRenderer::Instance().CreateBitmapFromFile(L"../Resource/cat.png", *m_SubCelBitmapPtr[i].GetAddressOf());
    }*/
}

void TransformPracticeScene::OnResize(int width, int height)
{ 
    // ������ ũ�� ���� �� ī�޶��� ȭ�� ũ�⸦ ������Ʈ
    m_UnityCamera.SetScreenSize(width, height);
}

void TransformPracticeScene::ProcessKeyboardEvents()
{
    /*
    // Ŭ����
    if (InputManager::Instance().GetKeyPressed(VK_F1))
    {
        ClearBoxObjects();
    }

    if (InputManager::Instance().GetKeyPressed(VK_F2))
    {
        SetBoxSelfRotation();
    }


    // ī�޶� �̵� ó��, 
    static const std::vector<std::pair<int, Vec2>> kCameraMoves = {
      { VK_RIGHT, {  1.f,  0.f } },
      { VK_LEFT,  { -1.f,  0.f } },
      { VK_UP,    {  0.f,  1.f } },
      { VK_DOWN,  {  0.f, -1.f } },
    };

    // C++17���ʹ� structured binding�� ����Ͽ� �� �����ϰ� ǥ���� �� �ֽ��ϴ�.
    for (auto& [vk, dir] : kCameraMoves)
    {
        if (InputManager::Instance().GetKeyDown(vk))
        {
            m_UnityCamera.Move(dir.x, dir.y);
        }
    }

    // ù��° ���õ� �ڽ��� �̵�
    static const std::vector<std::pair<int, Vec2>> kBoxMoves = {
      { 'D', {  1.f,  0.f } }, // DŰ�� ������ �̵�
      { 'A', { -1.f,  0.f } }, // AŰ�� ���� �̵�
      { 'W', {  0.f,  1.f } }, // WŰ�� ���� �̵�
      { 'S', {  0.f, -1.f } }, // SŰ�� �Ʒ��� �̵�

    };

    for (auto& [vk, dir] : kBoxMoves)
    {
        if (InputManager::Instance().GetKeyDown(vk))
        {
            if (m_SelectedCelObjects.size())
                m_SelectedCelObjects.front()->Move(dir);
        }
    }

    // ù��° ���õ� �ڽ��� ȸ��
    if (InputManager::Instance().GetKeyDown(VK_SPACE) && !m_SelectedCelObjects.empty())
    {
        m_SelectedCelObjects.front()->Rotate(1.f); // ���� ������ ȸ��, // Ʈ������ ���� �ٲ�
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

    zoom = std::clamp(zoom, 0.1f, 100.f); // �ʹ� �۰ų� ũ�� �� ������ ����
    m_UnityCamera.SetZoom(zoom);
}   

void TransformPracticeScene::AddCelObjects(D2D1_POINT_2F point) { // ��ũ�� ��ǥ ȹ��

    static int solarCount = 0;
    MAT3X2F cameraTM = m_UnityCamera.GetViewMatrix(); // ī�޶� ��� (��)�� ȹ��
    cameraTM.Invert(); // ī�޶� ��� ������

    D2D1_POINT_2F worldPt = cameraTM.TransformPoint(point); // ���� �����͸� �޾ƿ�

    CelestialObj* pNewObj = nullptr;
    if (m_CelObjects.size() < SOLAR_SYSTEM) { // �ϴ��� �¾�踸
        pNewObj = new CelestialObj(m_CelBitmapPtr[solarCount++]); // ������Ʈ ����
    }
    else {
        pNewObj = new CelestialObj(m_BitmapPtr); // ������Ʈ ����
    }
    
    if (m_CelObjects.empty()) { pNewObj->SetLeader(true); } // ù ���� �� ���� ����

    pNewObj->SetPosition(Vec2(worldPt.x, worldPt.y)); // ���� ���������� ����

    m_CelObjects.push_back(pNewObj); // ������ Obj �迭�� �߰�
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

    std::list<BoxObject*> oldSelectedList = std::move(m_SelectedBoxObjects); // lvalue Ÿ���� rvalue Ÿ������ Ÿ��ĳ����

    std::cout << "size of oldSelectedList: " << oldSelectedList.size() << std::endl;


    for (auto& box : m_BoxObjects)
    {
        if (box->IsHitTest(point, cameraTM))
        {
            box->ToggleSelected();

            if (box->IsSelected()) m_SelectedBoxObjects.push_back(box); // ���� ���õ� �ڽ� �߰�
        }
    }

    // ���� ���� ���� �����ϰ� ���õ� �ڽ� ��� �籸��
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

    // ���� ���� ������Ʈ
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

    if (it == m_SelectedCelObjects.end()) return; // ���õ� �ڽ��� ������ ����

    (*it)->SetLeader(true); // ù��° �ڽ��� ������ ����

    if (m_SelectedCelObjects.size() < 2) return; // ���õ� �ڽ��� 2�� �̸��̸� ����

    while (it != m_SelectedCelObjects.end() && std::next(it) != m_SelectedCelObjects.end())
    {
        CelestialObj* parent = *it;
        CelestialObj* child = *(std::next(it));

        child->SetParent(parent);

        it++;

        std::cout << "�θ�: " << parent->GetTransform()->GetPosition().x << ", "
            << parent->GetTransform()->GetPosition().y
            << " �ڽ�: " << child->GetTransform()->GetPosition().x << ", "
            << child->GetTransform()->GetPosition().y << std::endl;
    }
}
