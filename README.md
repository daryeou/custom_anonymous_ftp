# Custom_Anonymous_FTP
커스텀 소켓을 이용한 모의 FTP 콘솔 응용 프로그램입니다.

## Features
- 명령 및 파일 교환은 TCP통신으로 이루어집니다.
- 서버는 새로운 클라이언트가 연결될 때마다 새로운 스레드를 생성합니다.
- 클라이언트는 서버에 파일을 송신하거나 파일을 수신받을 수 있습니다. (단 파일의 사용 중 수정이 이루어지는 상황일 경우, 에러 메시지 반환)
- 클라이언트는 파일 목록을 받을 수 있고, 파일을 지울수도 있으며, 루트 디렉토리 내에서 경로를 이동할 수 있습니다.
- 서버는 클라이언트가 사용한 명령어와 접속, 종료 시간을 로그로 작성합니다.
- 클라이언트에서 동시에 같은 파일에 접근하는 것을 방지하기 위해 Mutex를 사용합니다.
- 파일 송/수신 소켓과 명령 전달 소켓은 분리되어 동작합니다.

## Installation
~~None~~

## How to work
<img width="90%" src="https://user-images.githubusercontent.com/90737528/158813527-d98b32d3-4587-4114-b86e-cd53634e9e0f.png"/>
<img width="90%" src="https://user-images.githubusercontent.com/90737528/158813559-0b2856f5-989a-458b-8da4-f46c28830272.png"/>
<img width="90%" src="https://user-images.githubusercontent.com/90737528/158813600-91feb127-86a4-4192-bde5-1247c4d5af96.png"/>
<img width="90%" src="https://user-images.githubusercontent.com/90737528/158813641-6ecdf446-2862-447a-b42d-f7b9192af22c.png"/>
<img width="90%" src="https://user-images.githubusercontent.com/90737528/158813679-519bac46-6aba-4769-bfcd-f7246c666667.png"/>
<img width="90%" src="https://user-images.githubusercontent.com/90737528/158813701-1b5ec307-1316-4243-8f02-b8c041f2cbe6.png"/>

## Demo
~~None~~

## Snapshots
‘ls’명령 사용
![image](https://user-images.githubusercontent.com/90737528/158814348-45240f4d-d672-480c-be9c-22482224bff1.png)

‘get’ 명령어 사용
![image](https://user-images.githubusercontent.com/90737528/158814366-fa70dfe4-a264-467a-943f-73581e2fb3cc.png)

‘get’명령어 사용 중에 ‘rm’또는‘put’사용 시
![image](https://user-images.githubusercontent.com/90737528/158814382-a7f24c12-a43f-4151-8ed3-f97f2ba8eb7d.png)

‘put’명령어 사용 시
![image](https://user-images.githubusercontent.com/90737528/158814393-cfabe313-f5ad-4f4f-bbb4-0ad13206d4f1.png)

서버에 해당 파일이 없거나 같은 파일이 존재할 때
![image](https://user-images.githubusercontent.com/90737528/158814428-d824b1ed-a700-469b-bb0b-a7520bdc2f72.png)

디렉토리 이동
![image](https://user-images.githubusercontent.com/90737528/158814439-e8b8b5ef-2d94-403a-afe1-99c53894988a.png)

‘rm’명령어 사용
![image](https://user-images.githubusercontent.com/90737528/158814450-b9713616-ea75-4189-8661-ae60eb911bf1.png)

파일 사용중에 ‘rm’사용 시
![image](https://user-images.githubusercontent.com/90737528/158814458-1c5f980f-187c-4838-95df-d93c457d9a3c.png)

로그기록
![image](https://user-images.githubusercontent.com/90737528/158814473-a8ce1140-2515-4ec3-a88c-ffd2ba153ac1.png)


## How to use   
- Run `ftpclient` on Client PC and typing Server IP and Port
- Run `ftpserver` on Server PC and typing Port

Done

## Thanks
~~None~~

## License
```
MIT License

Copyright (c) 2022 harusiku

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```



