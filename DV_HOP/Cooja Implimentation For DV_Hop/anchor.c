#include "contiki.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "lib/random.h"
#include "net/rime.h"
#include <stdio.h>
#include "node-id.h"
#include "dev/leds.h"
#include "node-id.h"
#include <math.h>
#include "dev/cc2420.h"
#include "dev/cc2420_const.h"


#define anchors_num 3
#define MAX_NEIGHBORS 16
/* This structure holds information about neighbors. */
struct neighbor {
struct neighbor *next;
rimeaddr_t addr;
};
rimeaddr_t next_hop; uint8_t nbr_hop;

/* This structure holds information about database. */
struct database {
uint8_t type;
uint8_t  id;
float  x;
float  y;
uint8_t  hop_count;
float Av_Hop_Size;
} database;

struct database currently_information;
struct database *received_data_mote;
struct database routing_table[anchors_num-1];
int counter=0;


struct average_hop_size {
uint8_t type;
uint8_t  id;
float Av_Hop_Size;
} average_hop_size;

struct average_hop_size currently_average_hop_size;
struct average_hop_size *received_avh_data;
uint8_t advance_routing_table[anchors_num-1];
int counter_advance=0;

struct exploit {
uint8_t id;
float x;
float y;
} exploit;
struct exploit *received_exploit;

int ids_counter=0;


static struct broadcast_conn broadcast;
static struct unicast_conn unicast;
/*---------------------------------------------------------------------------*/
/* We first declare our processes. */
PROCESS(broadcast_process, "Broadcast process");
PROCESS(flooding_process, "Flooding Process");
PROCESS(unicast_process, "Unicast process");
PROCESS(blink_process, "LED blink process");
PROCESS(display_process, "Display process");
PROCESS(av_hopsize_process, "Average HopSize process");
/* The AUTOSTART_PROCESSES() definition specifices what processes to
   start when this module is loaded. We put both our processes
   there. */
AUTOSTART_PROCESSES(&broadcast_process, &unicast_process);
/*---------------------------------------------------------------------------*/
/* This function is called whenever a broadcast message is received. */
static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
received_data_mote = packetbuf_dataptr();
process_start(&blink_process, NULL);
/*  Registration of database information in the databases table */
if(received_data_mote->type==0){ 
int i=0; 
for(i=0;i<counter;i++){
if ((received_data_mote->id==routing_table[i].id) &&(received_data_mote->id!=0)){
if(received_data_mote->hop_count <routing_table[i].hop_count){
routing_table[i].hop_count=received_data_mote->hop_count; 
if(received_data_mote->id==1){
rimeaddr_copy(&next_hop, from);
nbr_hop=received_data_mote->hop_count;
}  
process_start(&flooding_process, NULL);           
}
else{
break;	 
}   
break;              
}
} 
if((i==counter)&&(received_data_mote->id!=node_id)&&(received_data_mote->id!=0)){
routing_table[counter]=*received_data_mote;counter++; ids_counter++;
if(received_data_mote->id==1){
rimeaddr_copy(&next_hop, from);
nbr_hop=received_data_mote->hop_count;
} 
process_start(&flooding_process, NULL);i=0;     
}
}
/*************************************************************/
 else if(received_data_mote->type==1){
 received_avh_data=packetbuf_dataptr();
 int i=0; 
 for(i=0;i<counter;i++){
 if ((received_avh_data->id==routing_table[i].id)){
 routing_table[i].Av_Hop_Size=received_avh_data->Av_Hop_Size;        
 process_start(&flooding_process, NULL);		   
 break;              
}
} 
}
}
/**************************************************************************************************/
/*---------------------------------------------------------------------------*/
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static void recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
   
received_exploit= packetbuf_dataptr();
if(node_id==1){
int dec_x = received_exploit->x; float frac_x = received_exploit->x - dec_x;  
int dec_y = received_exploit->y; float frac_y = received_exploit->y - dec_y;
printf("Node %u: x=%d.%02d, y=%d.%02d\n",received_exploit->id,dec_x,abs((int)(frac_x*100)),dec_y,abs((int)(frac_y*100)));
}
 else{
packetbuf_copyfrom(received_exploit, sizeof(struct exploit));
unicast_send(&unicast, &next_hop);
}
}
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
/*---------------------------------------------------------------------------*/
/*--------------------------unicast_process-----------------------------------------------*/
PROCESS_THREAD(unicast_process, ev, data)
{
PROCESS_EXITHANDLER(unicast_close(&unicast);)   
PROCESS_BEGIN();
unicast_open(&unicast, 146, &unicast_callbacks);
PROCESS_END();
}
/*---------------------------------broadcast_process------------------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data)
{
node_id_burn(1);
cc2420_set_txpower(2);
static struct etimer et1;
currently_information.id=node_id; currently_information.hop_count=1;currently_information.type=0;
currently_average_hop_size.id=node_id;currently_average_hop_size.type=1;
PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
PROCESS_BEGIN();
broadcast_open(&broadcast, 129, &broadcast_call);
switch(node_id){
case 1: currently_information.x =5; currently_information.y =3;break;
case 2: currently_information.x =7; currently_information.y =6;break;
case 3: currently_information.x =9; currently_information.y =3;break;
}

if (node_id==1){
leds_on(LEDS_YELLOW);
etimer_set(&et1, (CLOCK_SECOND)+random_rand() % (CLOCK_SECOND));
PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et1));
printf("anchor hussein k%u start broadcast\n",node_id);
packetbuf_copyfrom(&currently_information, sizeof(struct database));
broadcast_send(&broadcast);
leds_off(LEDS_YELLOW);
PROCESS_EXIT();
}
else if (node_id==2){
PROCESS_WAIT_EVENT_UNTIL(received_data_mote->id==1);
leds_on(LEDS_YELLOW);
etimer_set(&et1, (CLOCK_SECOND)+random_rand() % (CLOCK_SECOND));
PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et1));
printf("anchor %u start broadcast\n",node_id);
packetbuf_copyfrom(&currently_information, sizeof(struct database));
broadcast_send(&broadcast);
leds_off(LEDS_YELLOW);
PROCESS_EXIT();
}
else if (node_id==3){
PROCESS_WAIT_EVENT_UNTIL(received_data_mote->id==2);
leds_on(LEDS_YELLOW);
etimer_set(&et1, (CLOCK_SECOND)+random_rand() % (CLOCK_SECOND));
PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et1));
printf("anchor %u start broadcast\n",node_id);
packetbuf_copyfrom(&currently_information, sizeof(struct database));
broadcast_send(&broadcast);
leds_off(LEDS_YELLOW);
PROCESS_EXIT();
}
PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(flooding_process, ev, data)
{
static struct etimer et1;
PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
PROCESS_BEGIN();
broadcast_open(&broadcast, 129, &broadcast_call);
 /**********************************FLOODING_TYPE=ZERO********************************************/
if (received_data_mote->type==0){
etimer_set(&et1, (CLOCK_SECOND)+random_rand() % (CLOCK_SECOND));
PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et1));
received_data_mote->hop_count++;
packetbuf_copyfrom(*(&received_data_mote), sizeof(struct database));
broadcast_send(&broadcast);

if((ids_counter == anchors_num-1)){
etimer_set(&et1, (CLOCK_SECOND)+random_rand() % (CLOCK_SECOND));
PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et1));
process_start(&av_hopsize_process, NULL);
PROCESS_EXIT();
printf("there is an error\n");
}      
}
/************************************FLOODUNG_TYPE=ONE******************************************/
else if(received_data_mote->type==1){
etimer_set(&et1, (CLOCK_SECOND)+random_rand() % (CLOCK_SECOND));
PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et1));
packetbuf_copyfrom(*(&received_avh_data), sizeof(struct average_hop_size));
broadcast_send(&broadcast);
}
 /************************************************************************************/

PROCESS_END();
}


/*---------------------------------------------------------------------------*/
 PROCESS_THREAD(blink_process, ev, data)
 {
 static struct etimer timer;
 PROCESS_BEGIN(); 
 leds_on(LEDS_GREEN);
 etimer_set(&timer, CLOCK_SECOND/4);
 PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
 leds_off(LEDS_GREEN);	 	 
 PROCESS_END();
 }


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(av_hopsize_process, ev, data)
{
static struct etimer et1;
PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
PROCESS_BEGIN();
PROCESS_WAIT_EVENT_UNTIL(node_id==1 || received_data_mote->id==1);
broadcast_open(&broadcast, 129, &broadcast_call);

if (node_id==1){
leds_on(LEDS_YELLOW);
etimer_set(&et1, (CLOCK_SECOND)+random_rand() % (CLOCK_SECOND));
PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et1));
printf("anchor %u start average_hop_size \n",node_id);
/*********************************************************************/
int dec_x; float frac_x;
currently_average_hop_size.type=1;
currently_average_hop_size.id=node_id;
float numenator=0, denominator=0, sum_numenator=0, sum_denominator=0, sum_hopsize=0;
int j=0;
for(j=0;j<counter;j++){
numenator=sqrt(pow(currently_information.x-routing_table[j].x,2)+pow(currently_information.y-routing_table[j].y,2));
denominator=routing_table[j].hop_count;    
sum_numenator+=numenator;
sum_denominator+=denominator;  
}
sum_hopsize=(float)sum_numenator/(float)sum_denominator;
currently_average_hop_size.Av_Hop_Size=(float)sum_hopsize;
/*************************************************************************************/

packetbuf_copyfrom(&currently_average_hop_size, sizeof(struct average_hop_size));
broadcast_send(&broadcast);
leds_off(LEDS_YELLOW);
PROCESS_EXIT();
}

  else if (node_id==2){
PROCESS_WAIT_EVENT_UNTIL(received_avh_data->id==1);
leds_on(LEDS_YELLOW);
etimer_set(&et1, (CLOCK_SECOND)*2+random_rand() % (CLOCK_SECOND));
PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et1));
printf("anchor %u start average_hop_size \n",node_id);
/*********************************************************************/
int dec_x; float frac_x;
currently_average_hop_size.type=1;
currently_average_hop_size.id=node_id;
float numenator=0, denominator=0, sum_numenator=0, sum_denominator=0, sum_hopsize=0;
int j=0;
for(j=0;j<counter;j++){
numenator=sqrt(pow(currently_information.x-routing_table[j].x,2)+pow(currently_information.y-routing_table[j].y,2));
denominator=routing_table[j].hop_count;    
sum_numenator+=numenator;
sum_denominator+=denominator;  
}
sum_hopsize=(float)sum_numenator/(float)sum_denominator;
currently_average_hop_size.Av_Hop_Size=(float)sum_hopsize;
/*************************************************************************************/
packetbuf_copyfrom(&currently_average_hop_size, sizeof(struct average_hop_size));
broadcast_send(&broadcast);
leds_off(LEDS_YELLOW);
PROCESS_EXIT();
}
 else if (node_id==3){
PROCESS_WAIT_EVENT_UNTIL(received_avh_data->id==2);
leds_on(LEDS_YELLOW);
etimer_set(&et1, (CLOCK_SECOND)*2+random_rand() % (CLOCK_SECOND));
PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et1));
printf("anchor %u start average_hop_size \n",node_id);
/*********************************************************************/
int dec_x; float frac_x;
currently_average_hop_size.type=1;
currently_average_hop_size.id=node_id;
float numenator=0, denominator=0, sum_numenator=0, sum_denominator=0, sum_hopsize=0;
int j=0;
for(j=0;j<counter;j++){
numenator=sqrt(pow(currently_information.x-routing_table[j].x,2)+pow(currently_information.y-routing_table[j].y,2));
denominator=routing_table[j].hop_count;    
sum_numenator+=numenator;
sum_denominator+=denominator;  
}
sum_hopsize=(float)sum_numenator/(float)sum_denominator;
currently_average_hop_size.Av_Hop_Size=(float)sum_hopsize;
/*************************************************************************************/

packetbuf_copyfrom(&currently_average_hop_size, sizeof(struct average_hop_size));
broadcast_send(&broadcast);
leds_off(LEDS_YELLOW);
PROCESS_EXIT();
}
PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(display_process, ev, data)
{ 
PROCESS_BEGIN(); 
int j=0;
for(j=0;j<counter;j++) {
printf("mote=%u,Nbr_hop=%u/",routing_table[j].id,routing_table[j].hop_count);
}
printf("\n");
PROCESS_END();
}
/*---------------------------------------------------------------------------*/
