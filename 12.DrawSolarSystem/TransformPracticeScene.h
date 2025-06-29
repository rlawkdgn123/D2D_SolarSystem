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

    void OnMouseLButtonDown(D2D1_POINT_2F point) override; // ���� ���� �̻��

    void OnMouseRButtonDown(D2D1_POINT_2F point) override; // ���� ���� �̻��

    void OnMouseWheel(int delta, D2D1_POINT_2F pos) override;


    void AddCelObjects(D2D1_POINT_2F point);

    void AddCelObjects(D2D1_POINT_2F point, E_CelestialObj celObj);

    void AddCelObjects(D2D1_POINT_2F point, E_Satellite satObj);

    void ClearCelObjects();

    void SelectCelObject(D2D1_POINT_2F point); // ���� ���� �̻��

    void SetCelSelfRotation();

    void UpdateRelationship(); // ���� ���� �̻��

    UnityCamera m_UnityCamera;

    ComPtr<ID2D1Bitmap1> m_BitmapPtr; // ��Ʈ�� �⺻�� (�����)

    // �¾�� õü : �¾�,������ȭ����õ��
    ComPtr<ID2D1Bitmap1> m_CelBitmapPtr[9]; // ��Ʈ�� COM��ü

    // ���� : ����(��), ȭ��(������, ���̸�), ��(������, �̿�, ���ϸ޵�, Į������),�伺(Ÿ��ź), õ�ռ�(�̶���), �ؿռ�(Ʈ����)
    ComPtr<ID2D1Bitmap1> m_SatBitmapPtr[13];

    std::vector<CelestialObj*> m_CelObjects; // õü ������Ʈ ����Ʈ

    std::list<CelestialObj*> m_SelectedCelObjects; // ���õ� õü ��ü��

}; 

