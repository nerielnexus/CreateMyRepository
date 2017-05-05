/**-----------------------------------------------------------------------------
* \brief 인덱스버퍼 생성
* 파일: IndexBuffer.cpp
*
* 설명: 인덱스 버퍼(Index Buffer)란 정점을 보관하기 위한 정점버퍼(VB)처럼
*       인덱스를 보관하기위한 전용 객체이다. D3D 학습예제에는 이러한 예제가
*       IB를 사용한 예제가 없기 때문에 새롭게 추가한 것이다.
*------------------------------------------------------------------------------
*/
#include <d3d9.h>
#include <d3dx9.h>
#include <iostream>
#include <fstream>
#include <vector>



/**-----------------------------------------------------------------------------
*  전역변수
*------------------------------------------------------------------------------
*/
LPDIRECT3D9             g_pD3D = NULL; /// D3D 디바이스를 생성할 D3D객체변수
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; /// 렌더링에 사용될 D3D디바이스
LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL; /// 정점을 보관할 정점버퍼
LPDIRECT3DINDEXBUFFER9	g_pIB = NULL; /// 인덱스를 보관할 인덱스버퍼


									  /// 사용자 정점을 정의할 구조체
struct CUSTOMVERTEX
{
	FLOAT x , y , z;	/// 정점의 변환된 좌표
	DWORD color;	/// 정점의 색깔
};

/// 사용자 정점 구조체에 관한 정보를 나타내는 FVF값
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)

struct MYINDEX
{
	WORD	_0 , _1 , _2;		/// 일반적으로 인덱스는 16비트의 크기를 갖는다.
								/// 32비트의 크기도 가능하지만 구형 그래픽카드에서는 지원되지 않는다.
};

CUSTOMVERTEX* myvertex = NULL;  // 동적할당할 CUSTOMVERTEX 배열
MYINDEX* myIB = NULL;  // 동적할당할  MYINDEX 배열


/* 파일에서 읽은 정보를 저장할 버퍼 */
FLOAT Buffer [ 256 ] = { 0.0f, };

/* 색상을 저장하는 버퍼 */
static DWORD ColorBuffer [ 8 ] = {
	0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffff00,
	0xff00ffff, 0xffff00ff, 0xff000000, 0xffffffff,
};

/*-------------------------------------------------------------------------------
 *	파일에서 데이터를 읽어 Buffer 에 저장하기
 *-------------------------------------------------------------------------------
 */
void FileToBuffer( )
{
	std::ifstream ifile;    // 파일을 읽어올 ifstream 객체를 생성
	ifile.open( "data.txt" , std::ifstream::in );    // ifstream 객체가 data.txt 를 읽기전용으로 읽어옴

	if ( !ifile.good() )    // 제대로 읽어오지 못했을 경우 함수 종료
		return;

	int count = 0;    // 버퍼의 위치를 가리킬 변수

	while ( !ifile.eof( ) )    // 파일의 끝(EOF) 에 다다를때까지 파일을 하나하나 읽어서 버퍼에 저장
	{
		ifile >> Buffer [ count ]; // >> : 파일에서 읽은 데이터를 저장할 버퍼에 맞게 변환해서 저장함
		count++;  // 저장을 했으니 배열의 다음 원소로 이동할 것
	}

	ifile.close( );    // 파일을 닫음
}


/**-----------------------------------------------------------------------------
* Direct3D 초기화
*------------------------------------------------------------------------------
*/
HRESULT InitD3D( HWND hWnd )
{
	/// 디바이스를 생성하기위한 D3D객체 생성
	if ( NULL == ( g_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
		return E_FAIL;

	/// 디바이스를 생성할 구조체
	/// 복잡한 오브젝트를 그릴것이기때문에, 이번에는 Z버퍼가 필요하다.
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory( &d3dpp , sizeof( d3dpp ) );
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	/// 디바이스 생성
	if ( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT , D3DDEVTYPE_HAL , hWnd ,
		 D3DCREATE_SOFTWARE_VERTEXPROCESSING ,
		 &d3dpp , &g_pd3dDevice ) ) )
	{
		return E_FAIL;
	}

	/// 컬링기능을 끈다.
	g_pd3dDevice->SetRenderState( D3DRS_CULLMODE , D3DCULL_CCW );

	/// Z버퍼기능을 켠다.
	g_pd3dDevice->SetRenderState( D3DRS_ZENABLE , TRUE );

	/// 정점에 색깔값이 있으므로, 광원기능을 끈다.
	g_pd3dDevice->SetRenderState( D3DRS_LIGHTING , FALSE );

	return S_OK;
}




/**-----------------------------------------------------------------------------
* 정점버퍼를 생성하고 정점값을 채워넣는다.
* 정점버퍼란 기본적으로 정점정보를 갖고있는 메모리블럭이다.
* 정점버퍼를 생성한 다음에는 반드시 Lock()과 Unlock()으로 포인터를 얻어내서
* 정점정보를 정점버퍼에 써넣어야 한다.
* 또한 D3D는 인덱스버퍼도 사용가능하다는 것을 명심하자.
* 정점버퍼나 인덱스버퍼는 기본 시스템 메모리외에 디바이스 메모리(비디오카드 메모리)
* 에 생성될수 있는데, 대부분의 비디오카드에서는 이렇게 할경우 엄청난 속도의 향상을
* 얻을 수 있다.
*------------------------------------------------------------------------------
*/
HRESULT InitVB( )
{
	/*	---------
	 *	개념 정리
	 *	---------
	 *
	 *	CUSTOMVERTEX* myvertex 는 자료형이 CUSTOMVERTEX 포인터 형이다.
	 *	myvertex 에 CUSTOMVERTEX 배열을 동적으로 할당할건데, CUSTOMVERTEX 배열은 각 원소가 CUSTOMVERTEX 구조체인
	 *	배열을 의미한다.
		
		인덱스	0					1					2					...
		자료값	[ x, y, z, color ]  [ x, y, z, color ]	[ x, y, z, color ]	...

	 *	즉 배열 myvertex 의 각 원소마다 Buffer 와 ColorBuffer 에서 값을 가져와 직접 입력해야한다.
	 *	그 작업은 아래 for 루프에서 해준다.

		if ( FAILED( g_pVB->Lock( 0, sizeof( (*myvertex) ) * vertex_count, ( void** ) &pVertices, 0 ) ) )
			return E_FAIL;
		memcpy( pVertices, myvertex, sizeof( (*myvertex) ) * vertex_count );

	 *	위의 구문에서 sizeof( (*myvertex) ) * 8 이 원본의 sizeof(vertices) 와 다른 점인데,
	 *	sizeof(myvertex) 는 sizeof( (CUSTOMVERTEX*) myvertex), 즉 포인터 myvertex 의 크기를 나타낸다.
	 *	sizeof(*myvertex) 는 sizeof( (CUSTOMVERTEX) *myvertex), 즉 myvertex 포인터가 가리키는 CUSTOMVERTEX 배열의
	 *	첫번째 원소의 크기를 나타낸다.
	 *	우리는 CUSTOMVERTEX	 배열의 모든 원소가 필요하고, 저 구문에서는 모든 원소의 크기가 필요하기 때문에
	 *	sizeof(*myvertex) 에 vertex_count 를 곱한 값을 넘겨주는 것이다.
	 */

	int vertex_count = Buffer[ 0 ];  // CUSTOMVERTEX 배열에 몇개의 원소가 들어갈것인지 정하는 변수. Buffer[0] 의 값을 받아온다.
	myvertex = new CUSTOMVERTEX[ vertex_count ];  // vertex_count 를 기반으로 CUSTOMVERTEX 배열을 동적할당

	for ( int i = 0; i < vertex_count; i++ )
	{
		myvertex[ i ].x = Buffer[ 2 + (3 * i) ];
		myvertex[ i ].y = Buffer[ 3 + (3 * i) ];
		myvertex[ i ].z = Buffer[ 4 + (3 * i) ];
		myvertex[ i ].color = ColorBuffer[ i ];
	}

	// 파일 내 자료들이 버퍼에 들어갔을때의, 자료와 인덱스 대조
	// 8 12		-1 1 1		1 1 1		1 1 -1		-1 1 -1		-1 -1 1		1 -1 1		1 -1 -1		-1 -1 -1
	//			 2 3 4		5 6 7		8 9 10		11 12 13	14 15 16	17 18 19	20 21 22	23 24 25

	/// 정점버퍼 생성
	/// 8개의 사용자정점을 보관할 메모리를 할당한다.
	/// FVF를 지정하여 보관할 데이터의 형식을 지정한다.
	if ( FAILED( g_pd3dDevice->CreateVertexBuffer( 8 * sizeof( CUSTOMVERTEX ),
		 0, D3DFVF_CUSTOMVERTEX,
		 D3DPOOL_DEFAULT, &g_pVB, NULL ) ) )
	{
		return E_FAIL;
	}

	/// 정점버퍼를 값으로 채운다. 
	/// 정점버퍼의 Lock()함수를 호출하여 포인터를 얻어온다.
	VOID* pVertices;
	if ( FAILED( g_pVB->Lock( 0, sizeof( (*myvertex) ) * vertex_count, ( void** ) &pVertices, 0 ) ) )
		return E_FAIL;
	memcpy( pVertices, myvertex, sizeof( (*myvertex) ) * vertex_count );
	g_pVB->Unlock( );
	return S_OK;
}


HRESULT InitIB( )
{

	/*	위의 InitVB 주석부분과 설명을 공유한다. */
	int index_count = Buffer[ 1 ];
	int moderate = Buffer[ 0 ] * 3 + 2;
	myIB = new MYINDEX[ index_count ];

	for ( int i=0; i < index_count; i++ )
	{
		myIB[ i ]._0 = Buffer[ moderate + 3*i ];
		myIB[ i ]._1 = Buffer[ moderate + (3 * i + 1) ];
		myIB[ i ]._2 = Buffer[ moderate + (3 * i + 2) ];
	}

	/* 파일 내 자료들이 버퍼에 들어갔을때의, 자료와 인덱스 대조
	* ... -1	 0  1  2	 0  2  3	 4  6  5	 4  7  6	 0  3  7	 0  7  4
	* ... 25	26 27 28	29 30 31	32 33 34	35 36 37	38 39 40	41 42 43
	*
	*			 1  5  6	 1  6  2	 3  2  6	 3  6  7	 0  4  5	 0  5  1d
	*			44 45 46	47 48 49	50 51 52	53 54 55	55 56 57	58 59 60
	*
	* Buffer[ 26 ] == Buffer[ Buffer[0]*3 + 2 ]
	*/


	/// 인덱스버퍼 생성
	/// D3DFMT_INDEX16은 인덱스의 단위가 16비트 라는 것이다.
	/// 우리는 MYINDEX 구조체에서 WORD형으로 선언했으므로 D3DFMT_INDEX16을 사용한다.
	if ( FAILED( g_pd3dDevice->CreateIndexBuffer( 12 * sizeof( MYINDEX ) , 0 , D3DFMT_INDEX16 , D3DPOOL_DEFAULT , &g_pIB , NULL ) ) )
	{
		return E_FAIL;
	}

	/// 인덱스버퍼를 값으로 채운다. 
	/// 인덱스버퍼의 Lock()함수를 호출하여 포인터를 얻어온다.
	VOID* pIndices;
	if ( FAILED( g_pIB->Lock( 0 , sizeof( *myIB ) * index_count , ( void** ) &pIndices , 0 ) ) )
		return E_FAIL;
	memcpy( pIndices , myIB , sizeof( *myIB ) * index_count );
	g_pIB->Unlock( );

	return S_OK;
}

/**-----------------------------------------------------------------------------
* 행렬 설정
*------------------------------------------------------------------------------
*/
VOID SetupMatrices( )
{
	/// 월드행렬
	D3DXMATRIXA16 matWorld;
	D3DXMatrixIdentity( &matWorld );							/// 월드행렬을 단위행렬으로 설정
	D3DXMatrixRotationY( &matWorld , GetTickCount( ) / 500.0f );	/// Y축을 중심으로 회전행렬 생성
	g_pd3dDevice->SetTransform( D3DTS_WORLD , &matWorld );		/// 디바이스에 월드행렬 설정

																/// 뷰행렬을 설정
	D3DXVECTOR3 vEyePt( 0.0f , 3.0f , -5.0f );
	D3DXVECTOR3 vLookatPt( 0.0f , 0.0f , 0.0f );
	D3DXVECTOR3 vUpVec( 0.0f , 1.0f , 0.0f );
	D3DXMATRIXA16 matView;
	D3DXMatrixLookAtLH( &matView , &vEyePt , &vLookatPt , &vUpVec );
	g_pd3dDevice->SetTransform( D3DTS_VIEW , &matView );

	/// 프로젝션 행렬 설정
	D3DXMATRIXA16 matProj;
	D3DXMatrixPerspectiveFovLH( &matProj , D3DX_PI / 4 , 1.0f , 1.0f , 100.0f );
	g_pd3dDevice->SetTransform( D3DTS_PROJECTION , &matProj );
}



/**-----------------------------------------------------------------------------
* 초기화 객체들 소거
*------------------------------------------------------------------------------
*/
VOID Cleanup( )
{
	if ( g_pIB != NULL )
		g_pIB->Release( );

	if ( g_pVB != NULL )
		g_pVB->Release( );

	if ( g_pd3dDevice != NULL )
		g_pd3dDevice->Release( );

	if ( g_pD3D != NULL )
		g_pD3D->Release( );

	if ( myvertex != NULL )
		delete [ ] myvertex;

	if ( myIB != NULL )
		delete [ ] myIB;
}




/**-----------------------------------------------------------------------------
* 화면 그리기
*------------------------------------------------------------------------------
*/
VOID Render( )
{
	/// 후면버퍼와 Z버퍼 초기화
	g_pd3dDevice->Clear( 0 , NULL , D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER , D3DCOLOR_XRGB( 0 , 0 , 255 ) , 1.0f , 0 );

	// 행렬설정
	SetupMatrices( );

	/// 렌더링 시작
	if ( SUCCEEDED( g_pd3dDevice->BeginScene( ) ) )
	{
		/// 정점버퍼의 삼각형을 그린다.
		/// 1. 정점정보가 담겨있는 정점버퍼를 출력 스트림으로 할당한다.
		g_pd3dDevice->SetStreamSource( 0 , g_pVB , 0 , sizeof( CUSTOMVERTEX ) );
		/// 2. D3D에게 정점쉐이더 정보를 지정한다. 대부분의 경우에는 FVF만 지정한다.
		g_pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
		/// 3. 인덱스버퍼를 지정한다.
		g_pd3dDevice->SetIndices( g_pIB );
		/// 4. DrawIndexedPrimitive()를 호출한다.
		g_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST , 0 , 0 , 8 , 0 , 12 );

		/// 렌더링 종료
		g_pd3dDevice->EndScene( );
	}

	/// 후면버퍼를 보이는 화면으로!
	g_pd3dDevice->Present( NULL , NULL , NULL , NULL );
}




/**-----------------------------------------------------------------------------
* 윈도우 프로시져
*------------------------------------------------------------------------------
*/
LRESULT WINAPI MsgProc( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam )
{
	switch ( msg )
	{
		case WM_DESTROY:
			Cleanup( );
			PostQuitMessage( 0 );
			return 0;
	}

	return DefWindowProc( hWnd , msg , wParam , lParam );
}




/**-----------------------------------------------------------------------------
* 프로그램 시작점
*------------------------------------------------------------------------------
*/
INT WINAPI WinMain( HINSTANCE hInst , HINSTANCE , LPSTR , INT )
{
	/// 윈도우 클래스 등록
	WNDCLASSEX wc = { sizeof( WNDCLASSEX ), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle( NULL ), NULL, NULL, NULL, NULL,
		L"D3D Tutorial", NULL };
	RegisterClassEx( &wc );

	/// 윈도우 생성
	HWND hWnd = CreateWindow( L"D3D Tutorial" , L"D3D Tutorial 07: IndexBuffer" ,
							  WS_OVERLAPPEDWINDOW , 100 , 100 , 300 , 300 ,
							  GetDesktopWindow( ) , NULL , wc.hInstance , NULL );

	FileToBuffer( );

	/// Direct3D 초기화
	if ( SUCCEEDED( InitD3D( hWnd ) ) )
	{
		/// 정점버퍼 초기화
		if ( SUCCEEDED( InitVB( ) ) )
		{
			/// 인덱스버퍼 초기화
			if ( SUCCEEDED( InitIB( ) ) )
			{
				/// 윈도우 출력
				ShowWindow( hWnd , SW_SHOWDEFAULT );
				UpdateWindow( hWnd );

				/// 메시지 루프
				MSG msg;
				ZeroMemory( &msg , sizeof( msg ) );
				while ( msg.message != WM_QUIT )
				{
					/// 메시지큐에 메시지가 있으면 메시지 처리
					if ( PeekMessage( &msg , NULL , 0U , 0U , PM_REMOVE ) )
					{
						TranslateMessage( &msg );
						DispatchMessage( &msg );
					}
					else
						/// 처리할 메시지가 없으면 Render()함수 호출
						Render( );
				}
			}
		}
	}

	/// 등록된 클래스 소거
	UnregisterClass( L"D3D Tutorial" , wc.hInstance );
	return 0;
}

