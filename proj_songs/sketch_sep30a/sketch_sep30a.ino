#include <LiquidCrystal.h>
#include <PinChangeInterrupt.h>
#include <PinChangeInterruptBoards.h>
#include <PinChangeInterruptPins.h>
#include <PinChangeInterruptSettings.h>
#include "songs.h"

/* lcd */
#define CONTRAST_PIN 6
#define CONTRAST_LEVEL 85  // 0-255 // 85
#define BACKLIGHT_PIN 9
#define BACKLIGHT_LEVEL 20  // 0-255 // 20

/* button */
#define BUTTON_PIN_0 A0
#define BUTTON_PIN_1 A1
#define BUTTON_PIN_2 A2


/* lcd */
LiquidCrystal lcd(12,11,2,3,4,5);  // RS, E, D4-D7

/* frame */
char line0[17];
char line1[17];
int line0_idx;
int line1_idx;
int line_sele;
int word_continue=0;
void getAFrame(char *strr,int *index,int len);

/* mode */
volatile int mode_sele=0;
volatile int song_idx=0;
volatile int next_song_flg=0;
volatile int last_song_flg=0;
void mode_0();
void mode_1();

/* button */
volatile unsigned long lastTime_0=0;
volatile unsigned long lastTime_1=0;
volatile unsigned long lastTime_2=0;
void buttonPress_0();
void buttonPress_1();
void buttonPress_2();

/*
 * 根据strr更新line0和line1。
 * 
 * char *strr - 某一句歌词
 * int *index - 从这句歌词的第几个字符开始
 * int len    - 这句歌词的长度
 */
void getAFrame(char *strr,int *index,int len)
{
  int word_start; // 单词起始下标
  int word_end;   // 单词终止下标
  int word_len;   // 单词长度
  line0_idx=0;
  line1_idx=0;
  line_sele=0;

  // 长单词相关处理
  if(word_continue==1)  // 需要接着放置长单词
  {
    for(int i=*index;i<len;i++)
    {
      // 开始接着放置长单词
      if(line_sele==0)  // 放入第一行
      {
        if(line0_idx==0)
        {
          line0[line0_idx]='-';
          line0_idx++;
        }
        line0[line0_idx]=strr[i];
        line0_idx++;
        if(line0_idx==16)  // 第一行已经放完
        {
          line0[line0_idx]='\0';
          line_sele=1;
        }
      }
      else if(line_sele==1)  // 放入第二行
      {
        if(line1_idx==0)
        {
          line1[line1_idx]='-';
          line1_idx++;
        }
        line1[line1_idx]=strr[i];
        line1_idx++;
        if(line1_idx==16)  // 第二行已经放完
        {
          line1[line1_idx]='\0';
          *index=i+1;
          return;
        }
      }

      if(i==len-1)  // 长单词结束，并且长单词是最后一个单词
      {
        word_continue=0;
        goto JUMP;
      }
      if(strr[i+1]==' ')  // 长单词结束，并且后面还有别的单词
      {
        word_continue=0;
        *index=i+1;
        break;
      }
    }
  }

  for(int i=*index;i<len;i++)
  {
    if(strr[i]!=' ' && (i==*index || strr[i-1]==' ')) word_start=i;  // 发现新单词起点，更新起始下标
    if(strr[i]!=' ' && (i==len-1 || strr[i+1]==' '))  // 发现新单词终点，更新终止下标，并把该单词放入/不放入屏幕中
    {
      word_end=i;
      word_len=word_end-word_start+1;
      if(word_len<=16)  // 单词长度小于等于16
      {
        if(line_sele==0)  // 第一行还可以放
        {
          if(line0_idx==0)  // 第一行还没有内容
          {
            int j;
            for(j=0;j<word_len;j++)  // 把单词放到第一行
            {
              line0[j]=strr[word_start+j];
            }
            line0_idx=j;
          }
          else if(line0_idx+word_len<16)  // 第一行有内容，且还可以接着放
          {
            int j;
            for(j=0;j<word_len+1;j++)  // 把单词放到第一行
            {
              if(j==0) line0[line0_idx+j]=' ';
              else line0[line0_idx+j]=strr[word_start+j-1];
            }
            line0_idx+=j;
          }
          else  // 第一行有内容，且不能接着放了
          {
            line_sele=1;  // 选第二行进行放置
            line0[line0_idx]='\0';
          }
        }
        
        if(line_sele==1)  // 需要放到第二行
        {
          if(line1_idx==0)  // 第二行还没有内容
          {
            int j;
            for(j=0;j<word_len;j++)  // 把单词放到第二行
            {
              line1[j]=strr[word_start+j];
            }
            line1_idx=j;
          }
          else if(line1_idx+word_len<16)  // 第二行有内容，且还可以接着放
          {
            int j;
            for(j=0;j<word_len+1;j++)  // 把单词放到第二行
            {
              if(j==0) line1[line1_idx+j]=' ';
              else line1[line1_idx+j]=strr[word_start+j-1];
            }
            line1_idx+=j;
          }
          else  // 第二行有内容，且不能接着放了，只能下一次再放了
          {
            *index=word_start;  // 下一次再重新放这个单词
            line1[line1_idx]='\0';
            return;
          }
        }
      }
      else if(word_len>16)  // 长单词相关处理 // 长单词，长度大于16，一行放不下
      {
        if(line_sele==0 && line0_idx==0)  // 第一第二行均为空
        {
          int word_finished=0;
          int j;
          for(j=0;j<16;j++)
            line0[j]=strr[word_start+j];
          line0[j]='\0';
          for(j=0;j<16;j++)
          {
            if(j==0) line1[j]='-';
            else
            {
              if(word_start+16+j-1==word_end)  // 第二行没放完单词就结束了
              {
                line1[j]=strr[word_start+16+j-1];
                word_finished=1;
                break;
              }
              else
              {
                line1[j]=strr[word_start+16+j-1];
              }
            }
          }
          if(word_finished)
          {
            line1_idx=j+1;
            line_sele=1;
          }
          else // word_finished==0
          {
            line1[j]='\0';
            *index=word_start+16+j-1;
            word_continue=1;
            return;
          }
        }
        else if(line1_idx==0) // 第一行不为空，第二行为空
        {
          int j;
          for(j=0;j<16;j++)
            line1[j]=strr[word_start+j];
          line1[j]='\0';
          line0[line0_idx]='\0';
          *index=word_start+j;
          word_continue=1;
          return;
        }
        else  // 第一第二行均不为空
        {
          *index=word_start;  // 下一次再重新放这个单词
          line1[line1_idx]='\0';
          return;
        }
      }
    }
  }

  JUMP:
  // 这句歌词放完了
  *index=len;
  if(line_sele==0) // 如果第一行还没放完
  {
      line0[line0_idx]='\0';  // 设置终止符
      line1[0]='\0';  // 第二行为空
  }
  else if(line_sele==1)
  {
      line1[line1_idx]='\0';
  }
}

void mode_0()
{
  lcd.clear();
  while(true)
  {
    lcd.setCursor(0,0);
    lcd.print("Happy Birthday");
    lcd.setCursor(0,1);
    lcd.print("to Huang Qi");
    unsigned long lastMillis=millis();
    while(millis()-lastMillis<=3000)
    {
      if(mode_sele!=0)
        return;
    }
  }
}

void mode_1()
{
  word_continue=0;
  next_song_flg=0;
  last_song_flg=0;
  RE:
  lcd.clear();
  while(true)
  {
    int strr_idx=0;
    char strr[100];
    while(strr_idx<songs_length[song_idx])
    {
      const char* const* song_t=(char* const*)pgm_read_word(&(songs[song_idx]));
      char* strr_t=(char*)pgm_read_word(&(song_t[strr_idx]));
      strcpy_P(strr,strr_t);

      int char_idx=0;
      int strr_len=strlen(strr);
      while(char_idx<strr_len)
      {
        getAFrame(strr,&char_idx,strr_len);
        lcd.setCursor(0,0);
        lcd.print(line0);
        lcd.setCursor(0,1);
        lcd.print(line1);
        unsigned long lastMillis;
        lastMillis=millis();
        while(millis()-lastMillis<=3500)
        {
          if(mode_sele!=1)
            return;
          if(next_song_flg==1)
          {
            next_song_flg=0;
            song_idx++;
            if(song_idx==songs_total) song_idx=0;
            goto RE;
          }
          if(last_song_flg==1)
          {
            last_song_flg=0;
            song_idx--;
            if(song_idx==-1) song_idx=songs_total-1;
            goto RE;
          }
        }

        lcd.clear();
        lastMillis=millis();
        while(millis()-lastMillis<=100)
        {
          if(mode_sele!=1)
            return;
        }
      }
      strr_idx++;
    }

    song_idx++;
    if(song_idx==songs_total)
      song_idx=0;
  }
}

void buttonPress_0()
{
  unsigned long currentTime=millis();
  if(currentTime-lastTime_0>200)
  {
    if(mode_sele==0)
    {
      mode_sele=1;
    }
    else if(mode_sele==1)
    {
      mode_sele=0;
    }
  }
  lastTime_0=currentTime;
}

void buttonPress_1()
{
  unsigned long currentTime=millis();
  if(currentTime-lastTime_1>200)
  {
    if(mode_sele==1)
    {
      next_song_flg=1;
    }
  }
  lastTime_1=currentTime;
}

void buttonPress_2()
{
  unsigned long currentTime=millis();
  if(currentTime-lastTime_2>200)
  {
    if(mode_sele==1)
    {
      last_song_flg=1;
    }
  }
  lastTime_2=currentTime;
}

void setup() {
  analogWrite(CONTRAST_PIN,CONTRAST_LEVEL);
  analogWrite(BACKLIGHT_PIN,BACKLIGHT_LEVEL);
  pinMode(BUTTON_PIN_0,INPUT_PULLUP);
  pinMode(BUTTON_PIN_1,INPUT_PULLUP);
  pinMode(BUTTON_PIN_2,INPUT_PULLUP);
  attachPCINT(digitalPinToPCINT(BUTTON_PIN_0),buttonPress_0,FALLING);
  attachPCINT(digitalPinToPCINT(BUTTON_PIN_1),buttonPress_1,FALLING);
  attachPCINT(digitalPinToPCINT(BUTTON_PIN_2),buttonPress_2,FALLING);
  lcd.begin(16,2);
  lcd.clear();
  // Serial.begin(9600);
}

void loop() {
  if(mode_sele==0) mode_0();
  else if(mode_sele==1) mode_1();
}
