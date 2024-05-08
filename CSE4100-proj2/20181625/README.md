1.	Task 1: Event-driven Approach
프로젝트에서 global data 영역에서 선언한 pool 구조체를 활용하여 concurrent server의 구현을 진행하게 된다. 
하나의 반복문에서, 선언된 pool을 순회하여 client가 서버에 요청한 connection request가 있는지 없는지 체크하고, connection request가 있었던 client들에 대해 client들의 요구에 맞춰 해당 요청에 대응되는 서비스를 concurrent하게 처리할 수 있게 된다. 

---------------------------------------------------------------------------

2.	Task 2: Thread-based Approach
Event-driven server와 달리, thread 기반으로 생성되는 pool을 기준으로 하게 된다. 
마찬가지로, 반복문에서 개별 thread를 핸들러를 통해 관리하게 되며, client의 connection request가 있었을 경우, client의 요구사항에 맞춰 서버가 그에 맞는 서비스를 진행하여 client에게 return 하게 된다. 
