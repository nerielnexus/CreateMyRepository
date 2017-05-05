/**-----------------------------------------------------------------------------
* \brief �ε������� ����
* ����: IndexBuffer.cpp
*
* ����: �ε��� ����(Index Buffer)�� ������ �����ϱ� ���� ��������(VB)ó��
*       �ε����� �����ϱ����� ���� ��ü�̴�. D3D �н��������� �̷��� ������
*       IB�� ����� ������ ���� ������ ���Ӱ� �߰��� ���̴�.
*------------------------------------------------------------------------------
*/
#include <d3d9.h>
#include <d3dx9.h>
#include <iostream>
#include <fstream>
#include <vector>



/**-----------------------------------------------------------------------------
*  ��������
*------------------------------------------------------------------------------
*/
LPDIRECT3D9             g_pD3D = NULL; /// D3D ����̽��� ������ D3D��ü����
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; /// �������� ���� D3D����̽�
LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL; /// ������ ������ ��������
LPDIRECT3DINDEXBUFFER9	g_pIB = NULL; /// �ε����� ������ �ε�������


									  /// ����� ������ ������ ����ü
struct CUSTOMVERTEX
{
	FLOAT x , y , z;	/// ������ ��ȯ�� ��ǥ
	DWORD color;	/// ������ ����
};

/// ����� ���� ����ü�� ���� ������ ��Ÿ���� FVF��
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)

struct MYINDEX
{
	WORD	_0 , _1 , _2;		/// �Ϲ������� �ε����� 16��Ʈ�� ũ�⸦ ���´�.
								/// 32��Ʈ�� ũ�⵵ ���������� ���� �׷���ī�忡���� �������� �ʴ´�.
};

CUSTOMVERTEX* myvertex = NULL;  // �����Ҵ��� CUSTOMVERTEX �迭
MYINDEX* myIB = NULL;  // �����Ҵ���  MYINDEX �迭


/* ���Ͽ��� ���� ������ ������ ���� */
FLOAT Buffer [ 256 ] = { 0.0f, };

/* ������ �����ϴ� ���� */
static DWORD ColorBuffer [ 8 ] = {
	0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffff00,
	0xff00ffff, 0xffff00ff, 0xff000000, 0xffffffff,
};

/*-------------------------------------------------------------------------------
 *	���Ͽ��� �����͸� �о� Buffer �� �����ϱ�
 *-------------------------------------------------------------------------------
 */
void FileToBuffer( )
{
	std::ifstream ifile;    // ������ �о�� ifstream ��ü�� ����
	ifile.open( "data.txt" , std::ifstream::in );    // ifstream ��ü�� data.txt �� �б��������� �о��

	if ( !ifile.good() )    // ����� �о���� ������ ��� �Լ� ����
		return;

	int count = 0;    // ������ ��ġ�� ����ų ����

	while ( !ifile.eof( ) )    // ������ ��(EOF) �� �ٴٸ������� ������ �ϳ��ϳ� �о ���ۿ� ����
	{
		ifile >> Buffer [ count ]; // >> : ���Ͽ��� ���� �����͸� ������ ���ۿ� �°� ��ȯ�ؼ� ������
		count++;  // ������ ������ �迭�� ���� ���ҷ� �̵��� ��
	}

	ifile.close( );    // ������ ����
}


/**-----------------------------------------------------------------------------
* Direct3D �ʱ�ȭ
*------------------------------------------------------------------------------
*/
HRESULT InitD3D( HWND hWnd )
{
	/// ����̽��� �����ϱ����� D3D��ü ����
	if ( NULL == ( g_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
		return E_FAIL;

	/// ����̽��� ������ ����ü
	/// ������ ������Ʈ�� �׸����̱⶧����, �̹����� Z���۰� �ʿ��ϴ�.
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory( &d3dpp , sizeof( d3dpp ) );
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	/// ����̽� ����
	if ( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT , D3DDEVTYPE_HAL , hWnd ,
		 D3DCREATE_SOFTWARE_VERTEXPROCESSING ,
		 &d3dpp , &g_pd3dDevice ) ) )
	{
		return E_FAIL;
	}

	/// �ø������ ����.
	g_pd3dDevice->SetRenderState( D3DRS_CULLMODE , D3DCULL_CCW );

	/// Z���۱���� �Ҵ�.
	g_pd3dDevice->SetRenderState( D3DRS_ZENABLE , TRUE );

	/// ������ ������ �����Ƿ�, ��������� ����.
	g_pd3dDevice->SetRenderState( D3DRS_LIGHTING , FALSE );

	return S_OK;
}




/**-----------------------------------------------------------------------------
* �������۸� �����ϰ� �������� ä���ִ´�.
* �������۶� �⺻������ ���������� �����ִ� �޸𸮺����̴�.
* �������۸� ������ �������� �ݵ�� Lock()�� Unlock()���� �����͸� ����
* ���������� �������ۿ� ��־�� �Ѵ�.
* ���� D3D�� �ε������۵� ��밡���ϴٴ� ���� ��������.
* �������۳� �ε������۴� �⺻ �ý��� �޸𸮿ܿ� ����̽� �޸�(����ī�� �޸�)
* �� �����ɼ� �ִµ�, ��κ��� ����ī�忡���� �̷��� �Ұ�� ��û�� �ӵ��� �����
* ���� �� �ִ�.
*------------------------------------------------------------------------------
*/
HRESULT InitVB( )
{
	/*	---------
	 *	���� ����
	 *	---------
	 *
	 *	CUSTOMVERTEX* myvertex �� �ڷ����� CUSTOMVERTEX ������ ���̴�.
	 *	myvertex �� CUSTOMVERTEX �迭�� �������� �Ҵ��Ұǵ�, CUSTOMVERTEX �迭�� �� ���Ұ� CUSTOMVERTEX ����ü��
	 *	�迭�� �ǹ��Ѵ�.
		
		�ε���	0					1					2					...
		�ڷᰪ	[ x, y, z, color ]  [ x, y, z, color ]	[ x, y, z, color ]	...

	 *	�� �迭 myvertex �� �� ���Ҹ��� Buffer �� ColorBuffer ���� ���� ������ ���� �Է��ؾ��Ѵ�.
	 *	�� �۾��� �Ʒ� for �������� ���ش�.

		if ( FAILED( g_pVB->Lock( 0, sizeof( (*myvertex) ) * vertex_count, ( void** ) &pVertices, 0 ) ) )
			return E_FAIL;
		memcpy( pVertices, myvertex, sizeof( (*myvertex) ) * vertex_count );

	 *	���� �������� sizeof( (*myvertex) ) * 8 �� ������ sizeof(vertices) �� �ٸ� ���ε�,
	 *	sizeof(myvertex) �� sizeof( (CUSTOMVERTEX*) myvertex), �� ������ myvertex �� ũ�⸦ ��Ÿ����.
	 *	sizeof(*myvertex) �� sizeof( (CUSTOMVERTEX) *myvertex), �� myvertex �����Ͱ� ����Ű�� CUSTOMVERTEX �迭��
	 *	ù��° ������ ũ�⸦ ��Ÿ����.
	 *	�츮�� CUSTOMVERTEX	 �迭�� ��� ���Ұ� �ʿ��ϰ�, �� ���������� ��� ������ ũ�Ⱑ �ʿ��ϱ� ������
	 *	sizeof(*myvertex) �� vertex_count �� ���� ���� �Ѱ��ִ� ���̴�.
	 */

	int vertex_count = Buffer[ 0 ];  // CUSTOMVERTEX �迭�� ��� ���Ұ� �������� ���ϴ� ����. Buffer[0] �� ���� �޾ƿ´�.
	myvertex = new CUSTOMVERTEX[ vertex_count ];  // vertex_count �� ������� CUSTOMVERTEX �迭�� �����Ҵ�

	for ( int i = 0; i < vertex_count; i++ )
	{
		myvertex[ i ].x = Buffer[ 2 + (3 * i) ];
		myvertex[ i ].y = Buffer[ 3 + (3 * i) ];
		myvertex[ i ].z = Buffer[ 4 + (3 * i) ];
		myvertex[ i ].color = ColorBuffer[ i ];
	}

	// ���� �� �ڷ���� ���ۿ� ��������, �ڷ�� �ε��� ����
	// 8 12		-1 1 1		1 1 1		1 1 -1		-1 1 -1		-1 -1 1		1 -1 1		1 -1 -1		-1 -1 -1
	//			 2 3 4		5 6 7		8 9 10		11 12 13	14 15 16	17 18 19	20 21 22	23 24 25

	/// �������� ����
	/// 8���� ����������� ������ �޸𸮸� �Ҵ��Ѵ�.
	/// FVF�� �����Ͽ� ������ �������� ������ �����Ѵ�.
	if ( FAILED( g_pd3dDevice->CreateVertexBuffer( 8 * sizeof( CUSTOMVERTEX ),
		 0, D3DFVF_CUSTOMVERTEX,
		 D3DPOOL_DEFAULT, &g_pVB, NULL ) ) )
	{
		return E_FAIL;
	}

	/// �������۸� ������ ä���. 
	/// ���������� Lock()�Լ��� ȣ���Ͽ� �����͸� ���´�.
	VOID* pVertices;
	if ( FAILED( g_pVB->Lock( 0, sizeof( (*myvertex) ) * vertex_count, ( void** ) &pVertices, 0 ) ) )
		return E_FAIL;
	memcpy( pVertices, myvertex, sizeof( (*myvertex) ) * vertex_count );
	g_pVB->Unlock( );
	return S_OK;
}


HRESULT InitIB( )
{

	/*	���� InitVB �ּ��κа� ������ �����Ѵ�. */
	int index_count = Buffer[ 1 ];
	int moderate = Buffer[ 0 ] * 3 + 2;
	myIB = new MYINDEX[ index_count ];

	for ( int i=0; i < index_count; i++ )
	{
		myIB[ i ]._0 = Buffer[ moderate + 3*i ];
		myIB[ i ]._1 = Buffer[ moderate + (3 * i + 1) ];
		myIB[ i ]._2 = Buffer[ moderate + (3 * i + 2) ];
	}

	/* ���� �� �ڷ���� ���ۿ� ��������, �ڷ�� �ε��� ����
	* ... -1	 0  1  2	 0  2  3	 4  6  5	 4  7  6	 0  3  7	 0  7  4
	* ... 25	26 27 28	29 30 31	32 33 34	35 36 37	38 39 40	41 42 43
	*
	*			 1  5  6	 1  6  2	 3  2  6	 3  6  7	 0  4  5	 0  5  1d
	*			44 45 46	47 48 49	50 51 52	53 54 55	55 56 57	58 59 60
	*
	* Buffer[ 26 ] == Buffer[ Buffer[0]*3 + 2 ]
	*/


	/// �ε������� ����
	/// D3DFMT_INDEX16�� �ε����� ������ 16��Ʈ ��� ���̴�.
	/// �츮�� MYINDEX ����ü���� WORD������ ���������Ƿ� D3DFMT_INDEX16�� ����Ѵ�.
	if ( FAILED( g_pd3dDevice->CreateIndexBuffer( 12 * sizeof( MYINDEX ) , 0 , D3DFMT_INDEX16 , D3DPOOL_DEFAULT , &g_pIB , NULL ) ) )
	{
		return E_FAIL;
	}

	/// �ε������۸� ������ ä���. 
	/// �ε��������� Lock()�Լ��� ȣ���Ͽ� �����͸� ���´�.
	VOID* pIndices;
	if ( FAILED( g_pIB->Lock( 0 , sizeof( *myIB ) * index_count , ( void** ) &pIndices , 0 ) ) )
		return E_FAIL;
	memcpy( pIndices , myIB , sizeof( *myIB ) * index_count );
	g_pIB->Unlock( );

	return S_OK;
}

/**-----------------------------------------------------------------------------
* ��� ����
*------------------------------------------------------------------------------
*/
VOID SetupMatrices( )
{
	/// �������
	D3DXMATRIXA16 matWorld;
	D3DXMatrixIdentity( &matWorld );							/// ��������� ����������� ����
	D3DXMatrixRotationY( &matWorld , GetTickCount( ) / 500.0f );	/// Y���� �߽����� ȸ����� ����
	g_pd3dDevice->SetTransform( D3DTS_WORLD , &matWorld );		/// ����̽��� ������� ����

																/// ������� ����
	D3DXVECTOR3 vEyePt( 0.0f , 3.0f , -5.0f );
	D3DXVECTOR3 vLookatPt( 0.0f , 0.0f , 0.0f );
	D3DXVECTOR3 vUpVec( 0.0f , 1.0f , 0.0f );
	D3DXMATRIXA16 matView;
	D3DXMatrixLookAtLH( &matView , &vEyePt , &vLookatPt , &vUpVec );
	g_pd3dDevice->SetTransform( D3DTS_VIEW , &matView );

	/// �������� ��� ����
	D3DXMATRIXA16 matProj;
	D3DXMatrixPerspectiveFovLH( &matProj , D3DX_PI / 4 , 1.0f , 1.0f , 100.0f );
	g_pd3dDevice->SetTransform( D3DTS_PROJECTION , &matProj );
}



/**-----------------------------------------------------------------------------
* �ʱ�ȭ ��ü�� �Ұ�
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
* ȭ�� �׸���
*------------------------------------------------------------------------------
*/
VOID Render( )
{
	/// �ĸ���ۿ� Z���� �ʱ�ȭ
	g_pd3dDevice->Clear( 0 , NULL , D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER , D3DCOLOR_XRGB( 0 , 0 , 255 ) , 1.0f , 0 );

	// ��ļ���
	SetupMatrices( );

	/// ������ ����
	if ( SUCCEEDED( g_pd3dDevice->BeginScene( ) ) )
	{
		/// ���������� �ﰢ���� �׸���.
		/// 1. ���������� ����ִ� �������۸� ��� ��Ʈ������ �Ҵ��Ѵ�.
		g_pd3dDevice->SetStreamSource( 0 , g_pVB , 0 , sizeof( CUSTOMVERTEX ) );
		/// 2. D3D���� �������̴� ������ �����Ѵ�. ��κ��� ��쿡�� FVF�� �����Ѵ�.
		g_pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
		/// 3. �ε������۸� �����Ѵ�.
		g_pd3dDevice->SetIndices( g_pIB );
		/// 4. DrawIndexedPrimitive()�� ȣ���Ѵ�.
		g_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST , 0 , 0 , 8 , 0 , 12 );

		/// ������ ����
		g_pd3dDevice->EndScene( );
	}

	/// �ĸ���۸� ���̴� ȭ������!
	g_pd3dDevice->Present( NULL , NULL , NULL , NULL );
}




/**-----------------------------------------------------------------------------
* ������ ���ν���
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
* ���α׷� ������
*------------------------------------------------------------------------------
*/
INT WINAPI WinMain( HINSTANCE hInst , HINSTANCE , LPSTR , INT )
{
	/// ������ Ŭ���� ���
	WNDCLASSEX wc = { sizeof( WNDCLASSEX ), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle( NULL ), NULL, NULL, NULL, NULL,
		L"D3D Tutorial", NULL };
	RegisterClassEx( &wc );

	/// ������ ����
	HWND hWnd = CreateWindow( L"D3D Tutorial" , L"D3D Tutorial 07: IndexBuffer" ,
							  WS_OVERLAPPEDWINDOW , 100 , 100 , 300 , 300 ,
							  GetDesktopWindow( ) , NULL , wc.hInstance , NULL );

	FileToBuffer( );

	/// Direct3D �ʱ�ȭ
	if ( SUCCEEDED( InitD3D( hWnd ) ) )
	{
		/// �������� �ʱ�ȭ
		if ( SUCCEEDED( InitVB( ) ) )
		{
			/// �ε������� �ʱ�ȭ
			if ( SUCCEEDED( InitIB( ) ) )
			{
				/// ������ ���
				ShowWindow( hWnd , SW_SHOWDEFAULT );
				UpdateWindow( hWnd );

				/// �޽��� ����
				MSG msg;
				ZeroMemory( &msg , sizeof( msg ) );
				while ( msg.message != WM_QUIT )
				{
					/// �޽���ť�� �޽����� ������ �޽��� ó��
					if ( PeekMessage( &msg , NULL , 0U , 0U , PM_REMOVE ) )
					{
						TranslateMessage( &msg );
						DispatchMessage( &msg );
					}
					else
						/// ó���� �޽����� ������ Render()�Լ� ȣ��
						Render( );
				}
			}
		}
	}

	/// ��ϵ� Ŭ���� �Ұ�
	UnregisterClass( L"D3D Tutorial" , wc.hInstance );
	return 0;
}
