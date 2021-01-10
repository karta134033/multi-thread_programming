---
tags: 平行程式設計
---

# Multi-thread Programming
<style>
.aligncenter {
    text-align: center;
}
img {
  border: 1px solid #ddd;
  border-radius: 4px;
  padding: 5px;  
}
</style>
## Q1
> In your write-up, produce a graph of speedup compared to the reference sequential implementation as a function of the number of threads used FOR VIEW 1. Is speedup linear in the number of threads used? In your writeup hypothesize why this is (or is not) the case? (You may also wish to produce a graph for VIEW 2 to help you come up with a good answer. Hint: take a careful look at the three-thread data-point.)

多執行緒的簡單實作方法Block Partition如下
```cpp=
void workerThreadStart(WorkerArgs *const args)
{
    int part = args->height / args->numThreads;
    int start = args->threadId * part;
    mandelbrotSerial(
        args->x0, args->y0, args->x1, args->y1, 
        args->width, args->height, start, part, 
        args->maxIterations, args->output
    );   
}
```
將總執行次數(預設為:1200)切分給數個threads執行，如: 2個threads會切分為[0~599, 599~1199], 3個threads會切分為[0~399, 400~799, 800~1199] 以此類推 

### -- 分析 --
實測預設結果(2個threads)可以發現確實有達到平行化加速的效果，加快了接近兩倍的時間(1.95倍)。
```s
[mandelbrot serial]:            [460.379] ms
Wrote image file mandelbrot-serial.ppm
[mandelbrot thread]:            [236.624] ms
Wrote image file mandelbrot-thread.ppm
                                (1.95x speedup from 2 threads)
```

然而當切換到3個threads的時候速度居然比2個threads還慢!
```s
[mandelbrot serial]:            [460.489] ms
Wrote image file mandelbrot-serial.ppm
[mandelbrot thread]:            [282.625] ms
Wrote image file mandelbrot-thread.ppm
                                (1.63x speedup from 3 threads)
```
切到4個threads看起來又變正常了，不過4個threads卻只有達到2.38倍左右的加速。
```s
[mandelbrot serial]:            [460.490] ms
Wrote image file mandelbrot-serial.ppm
[mandelbrot thread]:            [193.409] ms
Wrote image file mandelbrot-thread.ppm
                                (2.38x speedup from 4 threads)
```

#### 圖一
![](https://i.imgur.com/9p2Fuue.png)
執行結果的圖表如上，可以明顯觀察到結果並非線性，且離理想目標有一段距離。


### --檢視程式碼--
因為每一個thread都會呼叫$mandelbrotSerial()$這支function，而$mandelbrotSerial()$又會再for迴圈裡呼叫到$mandel()$這支function，這兩個function極有可能是thread加速無法達到極速的瓶頸，以下分別探討這兩個function。
1. $mandelbrotSerial()$
    ```cpp=
    void mandelbrotSerial(
        float x0, float y0, float x1, float y1,
        int width, int height,
        int startRow, int totalRows,
        int maxIterations,
        int output[])
    {
      float dx = (x1 - x0) / width;
      float dy = (y1 - y0) / height;

      int endRow = startRow + totalRows;

      for (int j = startRow; j < endRow; j++)
      {
        for (int i = 0; i < width; ++i)
        {
          float x = x0 + i * dx;
          float y = y0 + j * dy;

          int index = (j * width + i);
          output[index] = mandel(x, y, maxIterations);
        }
      }
    }
    ```
    由程式的第13行與第15行可以得知，$mandelbrotSerial()$僅會依據startRow與endRow變動迴圈執行的次數，而每個thread所切分到的執行大小是一樣的，理論上應該達到平行的加速與threads數成正比的結果，但再仔細看會發現有一個變因，那就是$mandel()$這支function。
    
2. $mandel()$
    ```cpp=
    static inline int mandel(float c_re, float c_im, int count)
    {
      float z_re = c_re, z_im = c_im;
      int i;
      for (i = 0; i < count; ++i)
      {

        if (z_re * z_re + z_im * z_im > 4.f)
          break;

        float new_re = z_re * z_re - z_im * z_im;
        float new_im = 2.f * z_re * z_im;
        z_re = c_re + new_re;
        z_im = c_im + new_im;
      }

      return i;
    }   
    ```
    $mandel()$會帶入三個參數分別為c_re, c_im, count，其中count會決定迴圈的執行次數，且count的傳入是**固定**的，因此可以先排除count對平行化的影響。
    仔細看迴圈內的內容會發現**第8行可能是影響平行化的關鍵**，因為只要符合這個$if$的條件即可跳脫迴圈而要構成此條件的關鍵就在我們傳入的參數c_re和 c_im。

### --檢視Thread的瓶頸--
利用$CycleTimer.h$給的方法來計算看看各個thread所消耗的時間，

***FOR VIEW 1***
* 2 Threads
    ```s
    thread0 takes [236.088] ms
    thread1 takes [236.933] ms
    ---
    thread0 takes [235.866] ms
    thread1 takes [236.935] ms
    ---
    thread0 takes [236.008] ms
    thread1 takes [237.145] ms
    ---
    thread0 takes [235.585] ms
    thread1 takes [236.732] ms
    ---
    thread0 takes [235.712] ms
    thread1 takes [236.861] ms
    ---
    [mandelbrot thread]:            [236.959] ms
    Wrote image file mandelbrot-thread.ppm
                            (1.95x speedup from 2 threads)
    ```
    兩個thread之間執行的時間是差不多的

* 3 Threads
    ```s
    thread0 takes [93.499] ms
    thread2 takes [93.752] ms
    thread1 takes [282.856] ms  #
    ---
    thread0 takes [93.187] ms
    thread2 takes [93.796] ms
    thread1 takes [282.777] ms  #
    ---
    thread0 takes [93.182] ms
    thread2 takes [93.832] ms
    thread1 takes [282.866] ms  #
    ---
    thread0 takes [93.308] ms
    thread2 takes [93.910] ms
    thread1 takes [282.756] ms  #
    ---
    thread0 takes [93.201] ms
    thread2 takes [93.805] ms
    thread1 takes [282.893] ms  #
    ---
    [mandelbrot thread]:            [282.873] ms 
    Wrote image file mandelbrot-thread.ppm
                            (1.63x speedup from 3 threads)
    ```
    可以發現thread1是造成程式無法最佳化平行的瓶頸!
    代表說以原本的切分方式去執行會使位在中間切分部分的區域(thread1)產生瓶頸。

* 4 Threads
    ```s
    thread0 takes [45.501] ms
    thread3 takes [45.922] ms
    thread1 takes [193.004] ms  #
    thread2 takes [193.596] ms  #
    ---
    thread0 takes [45.571] ms
    thread3 takes [46.019] ms
    thread1 takes [192.590] ms  #
    thread2 takes [193.342] ms  #
    ---
    thread0 takes [45.438] ms
    thread3 takes [45.912] ms
    thread1 takes [192.931] ms  #
    thread2 takes [193.594] ms  #
    ---
    thread0 takes [45.514] ms
    thread3 takes [45.940] ms
    thread1 takes [192.657] ms  #
    thread2 takes [193.424] ms  #
    ---
    thread0 takes [45.551] ms
    thread3 takes [45.926] ms
    thread1 takes [192.766] ms  #
    thread2 takes [193.549] ms  #
    ---
    [mandelbrot thread]:            [193.472] ms
    Wrote image file mandelbrot-thread.ppm
                            (2.38x speedup from 4 threads)
    ```
    根據*3 Threads*的結果我們可以發現在*4 Threads*有一樣的問題，在中間切分部分的區域產生瓶頸，代表在***VIEW 1***時中間區段帶入的x, y會耗費大部分時間(相比於前、後段)在$mandel()$迭代。
    
    #### --檢視圖片--
    將執行的起始位置分別切分為[0~399, 400~799, 800~1199]可以分別跑出如下圖的結果
    ![](https://i.imgur.com/8GvO6NI.png)
    可以看到這三張圖也代表著Thread0, Thread1, Thread2處理的圖片。
    中間的圖片代表瓶頸的位置，對比左右側兩張圖可以發現白色區塊比較多，對應的數值為$mandel()$最後回傳的$i$值，**白色對應灰階的數字為255，正好為$mandel()$回傳的極值**(若count以256帶入)。
    根據圖片與以上的分析可以得知，**只要白色區域越多計算越耗時**。
        
***FOR VIEW 2***
* 2 Threads
    ```s
    thread1 takes [114.644] ms
    thread0 takes [162.354] ms  #
    ---
    thread1 takes [114.347] ms
    thread0 takes [162.109] ms  #
    ---
    thread1 takes [114.588] ms
    thread0 takes [162.478] ms  #
    ---
    thread1 takes [114.438] ms
    thread0 takes [162.225] ms  #
    ---
    thread1 takes [114.478] ms
    thread0 takes [162.168] ms  #
    ---
    [mandelbrot thread]:            [162.142] ms
    Wrote image file mandelbrot-thread.ppm
                                    (1.67x speedup from 2 threads)
    ```
    因為***VIEW 2***在$main()$裡呼叫了$scaleAndShift()$，對預設的x0, x1, y0, y1做scale與位移，**間接** 影響到$mandelbrotSerial()$ 17、18行對x、y所做的計算，使得$mandel()$的演算法結果有所變動，也改變了瓶頸的位置。

* 3 Threads
    ```s
    thread2 takes [74.479] ms
    thread1 takes [79.653] ms
    thread0 takes [124.814] ms  #
    ---
    thread2 takes [74.436] ms
    thread1 takes [79.594] ms
    thread0 takes [124.731] ms  #
    ---
    thread2 takes [74.456] ms
    thread1 takes [79.594] ms
    thread0 takes [124.480] ms  #
    ---
    thread2 takes [74.481] ms
    thread1 takes [79.577] ms
    thread0 takes [124.555] ms  #
    ---
    thread2 takes [74.357] ms
    thread1 takes [79.522] ms
    thread0 takes [124.340] ms  #
    ---
    [mandelbrot thread]:            [124.374] ms
    Wrote image file mandelbrot-thread.ppm
                                    (2.18x speedup from 3 threads)
    ```

* 4 Threads
    ```s
    thread3 takes [57.600] ms
    thread1 takes [59.899] ms
    thread2 takes [60.127] ms
    thread0 takes [106.028] ms  #
    ---
    thread3 takes [57.444] ms
    thread1 takes [59.549] ms
    thread2 takes [60.085] ms
    thread0 takes [106.157] ms  #
    ---
    thread3 takes [57.477] ms
    thread1 takes [59.535] ms
    thread2 takes [60.078] ms
    thread0 takes [105.927] ms  #
    ---
    thread3 takes [57.563] ms
    thread1 takes [59.570] ms
    thread2 takes [60.106] ms
    thread0 takes [105.960] ms  #
    ---
    thread3 takes [57.436] ms
    thread1 takes [59.534] ms
    thread2 takes [60.099] ms
    thread0 takes [105.969] ms  #
    ---
    [mandelbrot thread]:            [105.964] ms
    Wrote image file mandelbrot-thread.ppm
                            (2.55x speedup from 4 threads)
    ```
    由*2,3,4 Threads*的結果可以發現，計算較耗時的區域被往前移了(原本為中間的區段)。
    #### --檢視圖片--
    將執行的起始位置分別切分為[0~399, 400~799, 800~1199]可以寫出如下圖的結果
    ![](https://i.imgur.com/qZpYQh0.png)
    可以觀察到大部分的白色移到上半部，代表瓶頸位移到前半部的計算，實驗後的結果也可得知Thread 0的計算負擔會上升。


#### 圖二
耗費時間
![](https://i.imgur.com/EFDZ5FR.png)
***VIEW 2***對比於***VIEW 1***的結果，在時間上確實有跟著thread的上升而下降，但離理想的遞減目標***VIEW 2 Ideal***還有一段距離，原因跟***VIEW 1***是一樣的，都是因為有瓶頸的問題，只是***VIEW 2***總體的計算時間比較少，且切分區域的計算量差異沒有***VIEW 1***來的顯著。

## Q2: 
>How do your measurements explain the speedup graph you previously created?

由[圖一](#####圖一)可觀察到，在***VIEW1***時會因為中間區端的工作量較龐大，而造成thread的加速產生瓶頸，我們可以以圖片來做簡單的解釋。
在工作站的系統上使用的是 Intel(R) Core(TM) i5-7500 CPU @ 3.40GHz 4核心的處理器，因此我們圖片以4核作為解釋的基準。
### --圖示--
* 2 Threads
    根據之前分析的結果可得知，2個threads彼此被分配到的工作量是差不多的，所以理論上是可以加速到接近2倍的速度，也確實有達到近似值(1.95倍)，若以圖片來解釋:
    ![](https://i.imgur.com/YE36AEv.png)
    兩個task的**顏色相同代表工作的負擔是一樣的**，在CPU內是可以完美被分配到兩個Core上執行
    <p class="aligncenter">
        <img src="https://i.imgur.com/HZ9vcwM.png" style="width: 70%"/>
    </p>
    至於該被分配到哪個core可能交由作業系統或硬體來決定。
    
* 3 Threads
    已知3個threads會有其中一個thread需要負責較繁重的工作，因此切分的工作會如下圖所示
    ![](https://i.imgur.com/hiNiNDo.png)
    紅色代表較繁重的工作(中間區段的計算)，橘色分別代表前後區段。
    分配的結果可能如下圖
    <p class="aligncenter">
        <img src="https://i.imgur.com/Kor87NH.png" style="width: 70%"/>
    </p>
    可以看到左下角的Core負責較繁重的工作，也因此計算耗時相較於另外兩個Core更為耗時，當差異大到一定程度後將會嚴重影響效能，如之前的結果分析[圖一](#####圖一)在加速上反而小於2個Threads。
* 4 Threads  
    跟3 Threads的結果很類似，不過因為是4個Core去分配工作，所以可以將原先中間區塊的部分再做劃分，如下圖所示
    ![](https://i.imgur.com/FtoMjYx.png)
    中間的Task(咖啡色)是計算量較大的區塊，在Core分配工作的結果可能如下圖
    <p class="aligncenter">
        <img src="https://i.imgur.com/awKF2U6.png" style="width: 70%"/>
    </p>
    左側的Core負責較多的計算，計算時間上也受到他們所影響，相比於3 Threads圖中左下Core的獨自奮鬥；4 Threads在兩個Core合作下，所需的耗時比3 Threads少上許多。

### --推論--
* 根據之前分析與圖示的結果得知，只要將中間複雜的部份切分出去給多個Core去執行，在時間上是會有提升的。
* 因為現在我們的$workerThreadStart()$的做法是根據"Thread"的數目去做資料的切割，那當Thread夠多使資料切的夠零散並分配給多個Core去執行，速度上很有可能會大幅提升。

### --實驗--
以下根據不同的Thread數目去計算各個Thread的執行結果，為了方便資料的切割，選擇的Thread數目都會剛好整除1200(預設的總執行次數)。

* 8 Threads
    ```s
    thread0 takes [7.300] ms                                      
    thread7 takes [7.399] ms                                      
    thread1 takes [38.345] ms                                     
    thread2 takes [95.790] ms                                     
    thread6 takes [83.738] ms                                     
    thread3 takes [125.478] ms                                    
    thread4 takes [118.143] ms                                    
    thread5 takes [123.153] ms
    ---
    thread0 takes [7.304] ms                                      
    thread7 takes [7.400] ms                                      
    thread1 takes [38.375] ms                                     
    thread6 takes [78.622] ms                                     
    thread5 takes [90.850] ms                                     
    thread2 takes [124.177] ms                                    
    thread3 takes [117.634] ms                                    
    thread4 takes [131.648] ms 
    ---
    thread0 takes [7.314] ms                                      
    thread7 takes [7.398] ms                                      
    thread1 takes [38.253] ms                                     
    thread6 takes [85.603] ms                                     
    thread5 takes [90.878] ms                                     
    thread2 takes [124.207] ms                                    
    thread3 takes [117.702] ms                                    
    thread4 takes [131.288] ms  
    ---
    thread0 takes [7.402] ms                                      
    thread7 takes [16.192] ms                                     
    thread1 takes [38.317] ms                                     
    thread6 takes [82.806] ms                                     
    thread5 takes [90.739] ms                                     
    thread2 takes [124.090] ms                                    
    thread3 takes [117.643] ms                                    
    thread4 takes [134.696] ms 
    ---
    thread0 takes [7.303] ms                                      
    thread7 takes [7.401] ms                                      
    thread1 takes [38.275] ms                                     
    thread6 takes [86.111] ms                                     
    thread5 takes [88.587] ms                                     
    thread2 takes [124.189] ms                                    
    thread3 takes [117.630] ms                                    
    thread4 takes [135.814] ms                                    
    [mandelbrot thread]:            [131.452] ms                  
    Wrote image file mandelbrot-thread.ppm                        
                                    (3.51x speedup from 8 threads)
    ```
    我們可以觀察到切分方式大致上會如下圖
    ![](https://i.imgur.com/mgtecyy.png)
    計算複雜程度由小到大依序為: 綠<藍<黃<橘。
    Task由左而右由上到下分別對應分給Thread0~Thread7。
    工作的分配上大致上會如下
    
    1. 首先，Task由小到大會先依序分配給各個Core
        <p class="aligncenter">
            <img src="https://i.imgur.com/dyH9IH0.png" style="width: 70%"/>
        </p>
    2. 還有剩下4個Task，可能會被隨機分配到Core，與之前的Task做並行處理。
        當分配的夠好的時候(如下圖)
        <p class="aligncenter">
            <img src="https://i.imgur.com/ZIJRX7P.png" style="width: 70%"/>
        </p>
        Core間的工作量越接近時，對平行化的加速會更顯著。
        
    3. 執行多次實驗後的加速程度如下圖
        ![](https://i.imgur.com/86G5Bci.png)
        以8Threads為例大約可達到了3.5倍的加速
        
* 16 Threads
    根據8 Threads的結果我們可以推斷16 Threads在更細小的工作切分下應該會再進一步加速。
    執行多次實驗後的結果如下
    ![](https://i.imgur.com/MXqDoYs.png)
    以8Threads為例大約可達到了3.65倍的加速
    
    
* 30 Threads
    最後將Threads開到最大可整除1200的數字"30"時執行多次實驗後的結果如下
    ![](https://i.imgur.com/kZa7he3.png)
    速度已經來到了約3.72倍，但也似乎沒辦法再大幅度增快了，因為工作站是4核的CPU，理論上的最高速只能達到4倍。
### --結論--
:::info
當Task被切分成越多份去執行時所產生的加速成果會越趨穩定。
:::
這結論也給了我們修改原本$workerThreadStart()$程式的idea。
    
## Q3
>In your write-up, describe your approach to parallelization and report the final 4-thread speedup obtained.
### --改良想法--
依據上面Q2所得出的結論，我們可以將原本的程式碼改為將Task做極大成度的劃分，將原本的1200執行次數分成1200個小的區塊，然後將這些區塊依序排給Thread。
* 以2 Threads為例排法為
    ```
    切分成1200個Task 代號依序為0~1199

                分配到的工作代號
    Thread0  0 2 4 6  ...  1198
    Thread1  1 3 5 7  ...  1199
    ```
* 以3 Threads為例排法為
    ```
    切分成1200個Task 代號依序為0~1199

                分配到的工作代號
    Thread0  0 3 6 9   ...  1197
    Thread1  1 4 7 10  ...  1198
    Thread2  2 5 8 11  ...  1199
    ```
這樣的排法有個優點，可以避免中間計算複雜的區塊只交給一個Thread處理。
且根據以上的結果我們可以推導出一個模式，
1. 每個Thread都是處理 1200/Thread個數 的資料。
2. 該Thread的下一個被分配到的工作代號剛好是上一個工作代號 + Thread個數，
    如3 Threads中，Thread 0處理的工作代號為0、3 (0 + Thread個數)、6 (3 + Thread個數)...

將以上的推導式化為程式後可以得到改良版的$workerThreadStart()$
```cpp=
void workerThreadStart(WorkerArgs *const args)
{    
    int args_remain = args->height % args->numThreads;
    int remain = args_remain ? args_remain : 0;  // 負責處理無法整除的餘數
    int part = args->height / args->numThreads;  // 對應以上的1.

    for(int i = 0; i < part; ++i) {
        int start = i * args->numThreads + args->threadId; // 對應以上的2.
        mandelbrotSerial(
            args->x0, args->y0, args->x1, args->y1, 
            args->width, args->height, start, 1, 
            args->maxIterations, args->output
        );
    }
    if (args->threadId + 1 <= remain) {
        int start = part * args->numThreads + args->threadId;
        mandelbrotSerial(
            args->x0, args->y0, args->x1, args->y1, 
            args->width, args->height, start, 1, 
            args->maxIterations, args->output
        );
    }
}
```
除了1.、2. 兩點外，改良版的$workerThreadStart()$也能對無法整除的Threads數目做處理。

### --分析--
執行改良後的$workerThreadStart()$結果如下
![](https://i.imgur.com/XwtjVUH.png)

4 Threads的結果為
```s
[mandelbrot serial]:            [460.891] ms
Wrote image file mandelbrot-serial.ppm
[mandelbrot thread]:            [121.658] ms
Wrote image file mandelbrot-thread.ppm
                                (3.79x speedup from 4 threads)
```
3.79倍對比於之前的討論是目前最好的結果，且根據以前的推斷，在切分到夠小的情況下執行的結果應該更為穩定，以下為執行多次的實驗結果
<p class="aligncenter">
    <img src="https://i.imgur.com/6FC59zp.png" style="width: 85%"/>
</p>

![](https://i.imgur.com/OgMDaWD.png)
結果應證了之前推斷，確實相當相當穩定，八次的結果都落在3.79或3.80倍。

## Q4
>Now run your improved code with eight threads. Is performance noticeably greater than when running with four threads? Why or why not? (Notice that the workstation server provides 4 cores 4 threads.)

系統能夠加速是根據Core個數而定(Intel 的[Hyper-Threading](https://zh.wikipedia.org/wiki/%E8%B6%85%E5%9F%B7%E8%A1%8C%E7%B7%92)除外)，之前[Q2](##Q2)會有Thread超過Core數目仍可使速度上升是因為原本的$workerThreadStart()$寫法是根據Thread數目去做工作劃分，會使速度提升純粹是工作被切分成多份後可以更平均的分配到各個Core(分配的決定權由作業系統處理)去執行。
改良後的$workerThreadStart()$已經**預先**由程式做工作的分配，所以Thread的上升超過硬體可支援的上限時必定會有些Threads在同一個Core並行處理，儘管新增一個Thread所需要的系統資源是小於Process的，但隨著Thread數量的上升，分配系統資源給Thread所產生的overhead以及Task需要在同一個Core並行處理勢必為影響效能、速度上也可能不再穩定。
### --實驗--
根據以上的論點可以做以下的實驗
* 8 Threads
* 16 Threads
* 30 Threads
 
對比4 Thread的結果
![](https://i.imgur.com/tHF24hz.png)
![](https://i.imgur.com/AKWS6wU.png)
8 Threads與16 Threads的極值雖然可以接近4 Threads，但確實相比於4 Threads較不穩定。
比較有趣的是在30 Threads的情況下結果又變得比較穩定，但能夠達到的極值離4 Threads有約0.03~0.04的差距，雖然結果變穩定但差距卻變大了。

### --推論--
以下為30 Threads的推論
* 因為Task切給多個Thread執行，所以當下一個Thread要進來處理的時候可以讓Core間的執行時間比較接近
    <p class="aligncenter">
        <img src="https://i.imgur.com/w9eCTPN.png" style="width: 70%"/>
    </p>
    計算所需的時間: 綠<黃<橘<紅
    原本的Task為橘色，因為在Core執行一段時間後計算所需的時間變小而變成綠色，所以下一個時間點的Task進來的時候
    <p class="aligncenter">
        <img src="https://i.imgur.com/0naicMj.png" style="width: 70%"/>
    </p>
    各個Core的Loading是接近的，也因為Next Task所需的計算時間是由原本的Task被30個Thread切分，所以各個Core的計算時間比較不會有差異。
* 對比於8 Thread
  因為***在8 Thread中各個Task所需計算的量是大於30Threads*** ，所以很有可能在某個時間點會有下面的結果
    <p class="aligncenter">
        <img src="https://i.imgur.com/M3rjyP7.png" style="width: 70%"/>
    </p>
    右上角的Core可能因為
    1. 比較晚接收到工作
    2. 作業系統上分配Task的順序問題
    3. 硬體的細微差異
    
    因此還沒做到綠色的程度。
    就如同A和B比賽跑，當都是跑50公尺的時候(如同30 Threads)差異不大，但當比賽跑200公尺(如同8 Threads)時A和B的差異可能就會上升。
    而這時候下個時間點的Task進來了
    <p class="aligncenter">
        <img src="https://i.imgur.com/VoUR1iH.png" style="width: 70%"/>
    </p>
    造成各個Core的計算時間的差異會依然存在，使得計算時間上有時候會不穩定。
    
根據以上的推論可得知，30 Threads執行時間會穩定主要是工作被大幅度的切分了，Core在執行該工作的時間差異會比較小，而時間上的極限達不到4 Threads的程度主要還是開一個Thread對系統還是有Overhead的存在，因此不是開越多對系統就有幫助，主要還是要看要解決的問題是要怎麼做劃分為主，若以原先的$workerThreadStart()$作法，開多個Thread是有幫助的，但若以**改良版**$workerThreadStart()$的作法，開多個Thread反而是沒有幫助的。