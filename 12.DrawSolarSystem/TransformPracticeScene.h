#pragma once
#include "OnlyForTestScene.h"
#include "Camera2D.h"
#include <list>
#include <vector>
#include <wrl/client.h>
#include <d2d1_1.h>

class CelestialObj;


enum class E_CelestialObj {
    Sun, Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, Neptune
};
enum class E_Satellite {
    Moon = 0, Phobos, Deimos, Io, Europa, Ganymede,  Callisto, Titan, Enceladus, Miranda, Titania, Oberon, Triton
};
class TransformPracticeScene : public OnlyForTestScene
{
public:
    TransformPracticeScene() = default;
    virtual ~TransformPracticeScene();
    
    void SetUp(HWND hWnd) override;

    void Tick(float deltaTime) override;

    void OnResize(int width, int height) override;

private:
    void BitMapSetUp();

    void ProcessKeyboardEvents();

    void OnMouseLButtonDown(D2D1_POINT_2F point) override; // 지금 예제 미사용

    void OnMouseRButtonDown(D2D1_POINT_2F point) override; // 지금 예제 미사용

    void OnMouseWheel(int delta, D2D1_POINT_2F pos) override;


    void AddCelObjects(D2D1_POINT_2F point);

    void AddCelObjects(D2D1_POINT_2F point, E_CelestialObj celObj);

    void AddCelObjects(D2D1_POINT_2F point, E_Satellite satObj);

    void ClearCelObjects();

    void SelectCelObject(D2D1_POINT_2F point); // 지금 예제 미사용

    void SetCelSelfRotation();

    void UpdateRelationship(); // 지금 예제 미사용

    UnityCamera m_UnityCamera;

    ComPtr<ID2D1Bitmap1> m_BitmapPtr; // 비트맵 기본값 (고양이)

    // 태양계 천체 : 태양,수금지화목토천해
    ComPtr<ID2D1Bitmap1> m_CelBitmapPtr[9]; // 비트맵 COM객체

    // 위성 : 지구(달), 화성(포보스, 데이모스), 목성(유로파, 이오, 가니메데, 칼리스토),토성(타이탄), 천왕성(미란다), 해왕성(트리톤)
    ComPtr<ID2D1Bitmap1> m_SatBitmapPtr[13];

    std::vector<CelestialObj*> m_CelObjects; // 천체 오브젝트 리스트

    std::list<CelestialObj*> m_SelectedCelObjects; // 선택된 천체 객체들

}; 

