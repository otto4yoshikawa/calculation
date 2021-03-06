// calc4.cpp : アプリケーションのエントリ ポイントを定義します。

// 2914-12-13 calc4.cpp : メイン プロジェクト ファイルです。
//  ver 4 ver 2017.12.11
// ver 2017.12.16
// ver 2018-1-15
// ver 2020-1-10  3 and 4 stack
#include "stdafx.h"
#include<Windows.h>
#include <iostream>
#include<string>
using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <io.h>
#include <string.h>
#include <assert.h>
//
FILE *file;

/* 各プランが独立という仮定 */
/* 逆は各プランの最大値 */
/* #define PROB_INDEPENDENT */
int pipe = 0;
int debuglevel = 0;
int cmp_packed(unsigned char *ptr0, unsigned char *ptr1, int size);
/* カードはint型で表し 0-12が A-K となる */
const char *cardsym = "A23456789TJQK";

#define NDECKS 4
#define NSTACK 3
#define MAXNODE 53
#define ALLBITSET ((1<<NSTACK)-1) /* 15 */

#define DATFILE3 "nstack3.dat"
#define DATFILE4 "nstack4.dat"
#define NPROCS 1

#define MAXCARD 52

typedef float Float;

/* Node は, 一つながり(連続していなくても良い)のカードの集まり */

typedef struct {
	int maxPath; /* nodes[0]からの最大距離 */
	int nCards;  /* 属するカードの個数 */
	int inNodes;   /* 入っていくノードの数 */
	int hashVal;
	int mask;  /* NSTACK bitのみ有効 すべての先祖について, n bit 目が0ならstack nに置ける ...*/
	int oldIndex;
	int nEdges; /* このノードから出る辺の個数 */
	int edges[MAXNODE];
} Node;
typedef struct {
	int type; /* DECK, STACK, MOVE */
	int dest, src;
} MMove;

#define DECK 0   /* srcは空 */
#define STACK 1  /* srcは空 */
#define XMOVE 2           // original MOVE

typedef struct {
	int nMoves;
	MMove moves[MAXCARD * 2];
} MMoves;
/* 1のビットの数 のテーブル */
char px[100][60];
const int maskCount[16] = {
	0, /* 0000 */
	1, /* 0001 */
	1, /* 0010 */
	2, /* 0011 */
	1, /* 0100 */
	2, /* 0101 */
	2, /* 0110 */
	3, /* 0111 */
	1, /* 1000 */
	2, /* 1001 */
	2, /* 1010 */
	3, /* 1011 */
	2, /* 1100 */
	3, /* 1101 */
	3, /* 1110 */
	4, /* 1111 */
};

/* まずmaxPathでソートすることを利用して, 入らないノードを取り除く */
#define UNUSED 100
const char *inputdata[] = {
	"94TQ3K6K29T85T4J646633T4252A588KKAAQ7J75AQ7397QJ289J",
	"2J772344K39TQ5KT686JJ3T55A94A873K6AAQ42T7K59J2QQ6898",
	"Q76AQT9T55QJ6T4A65K8Q8KK37923K2J45472269TJ38A9A4J387",
	"29KTA328K33JTJ9Q67TK97A5628J474558Q5A67QA4QJ394TK826",
	"2Q4J66J234T285Q85A5337TKAJ5986K97JQ7KA4287AK493T69QT",
	"5KT95A2788K6J53Q93Q7T9AJK2JT6K883QQ2746T43J5A92A4746",
	"595398K7T7JT369JT69733A8QQ548224K2K8J5476QTAA6KQAJ24",
	"9294Q4QJAT862QKJQ7K4T39A7385AK7625K6338A5J9852TJT746",
	"6A92A72554J8J7TAKQ67QKK4QT52539T98Q2J493J8T346K67A38",
	"A7563AAT897TQ4629QJ2759KK843988T622567KA45JKJJ34QQ3T",
	"8QA349A83AA672T9K3TJ7K8T57Q99T6262J6JK4K475QJ55482Q3",
	"J99Q435K7T6723A582624T5KQ8Q4AJA5T476K3JTA9K968Q3J782",
	"56A8T4579T53K2K83347QJJ9Q286A7JQQ46KT59K623J8A972TA4",
	"J353244AK2Q28KQ7J434599TKT388K7QA62J5T7985TQ6766AJ9A",
	"J68TAAT5784K7863K95K9Q9224Q42TT97JA3J863J64523AQ7QK5",
	"8K22863K2J5QTA973JJ79Q74T5442A86635K9594J3QK7AT86QTA",
	"686398J4QA9T2375T7JA39KQ74Q452AT8A84J53K629KQK756T2J",
	"6K48585J483TK9Q7738Q9K3562J579TA73TA6A9JQ6KA22Q244TJ",
	"982J32T5A9QJQ64575T293Q37647ATJ6296K5K4KK478JA8T83QA",
	"9T7K9QK24536TK3T5J75TJ227AQ6A78J8A46859KA3J32Q4684Q9",
	"AQ5K69832J4263868K2K85Q57JT9724T9J36TQ94ATJ3KQ77AA54",
	"8T3AK2JT832AQ7K9J24T58QQ7979T435Q26K5K4A85639JJ7A646",
	"69TJJ974K32697T295J75TQ5K2T65AA8K683KA4Q48AQ83437QJ2",
	"3587AQQ8JQ9T7KA92T6342A43KJJK249977Q5AT5KT8866J64532",
	"T2J7KT7933798A5JQ8T5A624466762AK3KJ8JA3K48Q49TQQ9525",
	"5Q389Q4KJK7K436J5QTAJ52K2AJ98Q8TT456A846A939673227T7",
	"4278J7425A8426A3T639Q6K993Q3T85TA72A695K5J4K7KQJT8QJ",
	"663574JTQ93QQKJJA64238J49278T329TQAA2K77KKT88469A555",
	"22TKKA9TJ5A3Q9747KQTJ6895JA654T3J238Q8476697K52A8Q34",
	"TJJ7K99Q4KKJ32377255A28TQ5QT6A76JA64K2A9TQ8384935846",
	"Q4Q6K6534A78A3Q9KAKJ75T75649233Q8947J6T982J5K2AJT28T",
	"AT7KQ2T368J285TJ387772Q364KQ94K6A294J394A595QT586JKA",
	"AK79TJ78Q2J524T83TQKJ3KT6794QAJ6K323598Q58A66472A945",
	"6225K7T35K2359Q8ATK394Q8J59QJ96248T7J68T4477AQA3K6AJ",
	"A775Q3592469J836874T6QKAA3K34TA25K426T92J98TJQ8KQ57J",
	"75T7QAA4662A8T296AK8QT492876385Q5J4523J9J33TQJKK94K7",
	"7JAA4TA86J65J725247K3TJK6K8TQ573QK24939Q389T652A4Q98",
	"Q9K45258J67Q34QK364T7AATA9Q79J4J268K7J582395TT68KA23",
	"55Q8Q7696673TTA3593K22QJ49A79J8KTKT6JQ8853AK74J2A442",
	"A6534A96QQA5T2K9JQ3TQT7TJJ6267732473K299K4848AKJ8855",
	"5K357A9T44A7KA82Q7653T42Q6Q9JTT824KJ396K9725JJA6838Q",
	"7A6QQTKQ6J29TJ5774697JJT2449K4823A53KA9T58Q32838A65K",
	"67JQ3J543TTT962K34Q89528A925T77K4A47668QA9J58QJ3KA2K",
	"562JA3AQ48K55QJ4A987K3T976QKT37TK6JQ93J28284572AT964",
	"57749352A8475AQJT3TQ9886626KA6AJ35QT29QTJ8K742J9KK34",
	"A35A72A3638456T2QKT6J9JQK24K67329J9874KAJ495TQQ78T85",
	"7A8KQAA3QTJ2678J3Q59K747Q23A996KJ48649558TJKT6T25324",
	"265J92J7Q8A9K4TK544Q238KQ8TT78AJ56397256JAA3694Q7K3T",
	"4574A658J239T4369377J2TT53K86JK8AJ826Q5AAKT9QQ49K7Q2",
	"JK37227T5365JQA9Q4K888A6J7TQ498K3J5442A5966AQ32TT7K9",
	"55T659KA92864587T474242A673Q8KQTJK9AQT7J8936J3Q3A2KJ",
	"7JJ66K7TTAQA2K9AJK9T568Q74QJ8Q3894235A42T4738359K526",
	"T35KQA63J9K356KQ92632K94T64QA4T7742J7J25A588J88Q97AT",
	"K768K82444T655K9J2726AJ973Q9TAJT25345A3879KJTQ3AQ68Q",
	"K96982TK3QJA637457736JTQ7T8JJTK4Q23QA89258525A964AK4",
	"K75J795QA256K8T287J8TJK4A46T3T6329QJ3853Q492AQ7A649K",
	"Q3928J76KK5T92A8QJQK65K433J984J3775T7A4A298Q6TA6T254",
	"K7Q4J57224J28Q6KJ567873K984568T3699A9T2JAAKQ4A353TQT",
	"5TTK897J62527672T4A2QT9Q9836584KJA3KKQ3Q6J7543A98JA4",
	"K3547KJQ94QA5A668466JT2774Q95KQ5T8A8K833T2A39J229TJ7",
	"T2A734A97JT685AQ46J987895K5K7J5Q2A96K3T3Q26J42T3K84Q",
	"66QQAK2294598K7KAJ4A7T8276JJ6K344T998735852JQ5TA3T3Q",
	"2537JJ439TT6TKA2642J568Q9K7599A7JA4865TAQQ73KQ4883K2",
	"K524532362A8JKJ4T525T8364984JTQKQ6QQ977JAT3699K7AA87",
	"785756A47Q29K56328QAKQKJ68A99J732236QJ849A4TK43T5TJT",
	"97Q72J2AT3Q45J539A8J4K65TTJ732943K47668TQQ5KAA6898K2",
	"T93K34Q577AAKQ76JT2Q36584KAAJJT96889J34T622K5875294Q",
	"72KT43K78A974AJJ5852A6995528J9368Q6Q4TQ2TKJTA73K463Q",
	"J933QAQA452K75QT9586744J3KJ9KT9J776658T2A8A638TKQ242",
	"59256TK8K3KK48Q2Q6793589JQT425TT3J2684734AA76A9QA7JJ",
	"7KA4379QJ3KQJT22453Q9853TJ229J4865K64A88K65977A6TATQ",
	"8T2J4969K623T5ATQ739J9TQQK58527686754KA842KJJQA7343A",
	"77T28A939T68QJ9353J7K4TQJKA52A446Q5K69Q6AJ3T285487K2",
	"76TK9T8K44987AT592J7J246J35AK33Q82A3Q27JQ6KA598T456Q",
	"4T494J2664Q55877A2TAKJ98Q8J59636Q2QJ3K3TK35A9T2K877A",
	"8J355T2TAJ4797JK25K8TA22KA793Q4J6846379QQTA6QK489536",
	"T8T98J2954Q83A23K47J736K5AAT68K3275AQ67QQTJJ64K42995",
	"36Q47Q7A42JTT3K3TJ74Q3KK46A828A9962599T5568KJ8275JQA",
	"452K8Q76J83J76T33Q43749T52625K897AKAQ9Q8T5KAA24T96JJ",
	"2948K734K85853TATJJ2J75395TQQ292AA8T6K4AQJ3K476667Q9",
	"83A6789976T3JK4388K5T5JQ7JAJTQ93Q7T42496A6Q52KK22A45",
	"7A83JAJ55K6QA7932984TK22676TQ24K9AJT795KJ84TQ543Q368",
	"J2TQ5942A935JA577563K892486JTK8JT7A6448TA9Q36KQ73KQ2",
	"A946A8T769245QKK6J85ATQ592T78Q9K2237J5JJA348763QK43T",
	"4T6A869KKJ4QJ9KKA4Q695288TAAJ553797Q73J2336T42Q285T7",
	"44A862J276772K8Q336JTA588Q3599TJ7T5K4K9AA5QJQ62T493K",
	"4J875KTT2QKA929JQ5Q8TA65K32773J7685K4864T3A6J2Q94A39",
	"63JA22943T7T9T9648KKJ797Q8Q85TA56JK6338Q5245QJA7K4A2",
	"736T563298823JK8TJA942JKQ7K7A8557Q52Q96Q9JK43T4AT64A",
	"J98TQ44J9A8A537Q8T53276875J4TA69QA2KJ2679K3T46KK53Q2",
	"68K85725QAJ5K2QA49334382K27Q9TA94JQ6686497J37TKTA5JT",
	"J4Q4TA25A99275K88475TTQQ78A3K36K35JAK9269J83667JQ42T",
	"T6AA9K552K4Q26TJ38338Q249T9KQA874Q9737J56KJTJ572648A",
	"36KQA7K4T59A8K762683Q64T7JJATA2947JK53Q959823J2QT548",
	"8325J92Q3753TK8Q892TKJ466A4A964A2KJTKJ47559QAQ63T877",
	"K58958QTJ399347K652244QAQJ8TJAK739QAT7AJ22668476T53K",
	"AJ4J923K5JK957AQA68K98A5J5K7Q4Q4822436928TTQ7T6T3637",
	"K7T6JA4K9KK2A56Q58TQ24J335A6256472Q397JQ8T38J989AT74",
	"3495832TQ888ATAT25JQ295K6777Q76699AJTA43534J4KKK62JQ",
	"37K5Q5463K8484A23J8J95Q84J3AQ69AT26T5T97K26Q2797TJAK" };

void readTest();

int cmpNode(Node *n0, Node *n1)
{
	if (n0->maxPath>n1->maxPath) return 1;
	else if (n0->maxPath<n1->maxPath) return -1;
	if (n0->nCards>n1->nCards) return 1;
	else if (n0->nCards<n1->nCards) return -1;
	if (n0->inNodes>n1->inNodes) return 1;
	else if (n0->inNodes<n1->inNodes) return -1;
	if (n0->hashVal>n1->hashVal) return 1;
	else if (n0->hashVal<n1->hashVal) return -1;
	if (maskCount[n0->mask]>maskCount[n1->mask]) return 1;
	else if (maskCount[n0->mask]<maskCount[n1->mask]) return -1;
	return 0;
}

typedef struct _State {
	int nNodes;
	Node nodes[MAXNODE];
} State;

void copyState(State *state0, State *state1)
{
	int i, nNodes, j, nEdges;
	Node *pNode0, *pNode1;
	nNodes = state0->nNodes = state1->nNodes;
	for (i = 0; i<nNodes; i++) {
		pNode0 = &(state0->nodes[i]);
		pNode1 = &(state1->nodes[i]);
		nEdges = pNode0->nEdges = pNode1->nEdges;
		pNode0->mask = pNode1->mask;
		pNode0->nCards = pNode1->nCards;
		for (j = 0; j<nEdges; j++)
			pNode0->edges[j] = pNode1->edges[j];
	}
}

void showState(State *state)
{
	int nNodes = state->nNodes;
	int i, j, nEdges;

	if (state->nodes[0].mask != 0) {
		abort();
	}
	for (i = 0; i<nNodes; i++) {
		putchar('[');
		printf("%d,%d,0x%x:", i, state->nodes[i].nCards, state->nodes[i].mask);
		nEdges = state->nodes[i].nEdges;
		for (j = 0; j<nEdges; j++) {
			printf("%d,", state->nodes[i].edges[j]);
		}
		putchar(']');
	}
	putchar('\n'); fflush(stdout);
}

void markDecendants(State *state, int node, int *decendants)
{
	int i, nEdges;

	if (decendants[node] != 0) return;
	decendants[node] = 1;
	nEdges = state->nodes[node].nEdges;
	for (i = 0; i<nEdges; i++)
		markDecendants(state, state->nodes[node].edges[i], decendants);
}

/* 0 は未チェック */
/* 1 は positive */
/* -1 は negative */
/* ancestors に入れると共に return value でも返す */

int markAncestors(State *state, int node, int iNode0, int *ancestors)
{
	int i, nEdges, r, cur_r;
	Node *pNode0;

	if (iNode0 == node) {
		ancestors[node] = -1;
		return 1;
	}
	if (ancestors[iNode0] != 0) return ancestors[iNode0];
	pNode0 = &(state->nodes[iNode0]);
	nEdges = pNode0->nEdges;
	for (r = -1, i = 0; i<nEdges; i++) {
		if (iNode0 == pNode0->edges[i]) {
			abort();
		}
		if (markAncestors(state, node, pNode0->edges[i], ancestors)>0) r = 1;
	}
	ancestors[iNode0] = r;
	return r;
}

void setMasks(State *state)
{
	int i, j, k;
	int mask0, path0;
	int nEdges0, nEdges1, nNodes;
	int iNode1, iNode2;
	Node *pNode0, *pNode1;
	int decendants[MAXNODE];

	nNodes = state->nNodes;
	for (i = 0; i<nNodes; i++) {
		mask0 = state->nodes[i].mask;
		memset(decendants, 0, sizeof(decendants));
		markDecendants(state, i, decendants);
		for (j = 0; j<nNodes; j++)
			if (decendants[j] != 0) {
				state->nodes[j].mask |= mask0;
			}
	}
}

#if 0
/* サイズが0のノードは取り除くことができる */
/* maxPath を UNUSEDにすることが削除を意味する */

void removeZeroNodes(State *state)
{
	int i, j, k, nNodes;
	Node *pNode0, *pNode1;
	int decendants[MAXNODE];
	int nEdges0, nEdges1;
	int iNode2;

	nNodes = state->nNodes;
	for (i = 0; i<nNodes; i++) {
		pNode0 = &(state->nodes[i]);
		nEdges0 = pNode0->nEdges;
		for (j = 0; j<nEdges0; j++) {
			pNode1 = &(state->nodes[pNode0->edges[j]]);
			if (pNode1->nCards == 0) {
				pNode1->maxPath = UNUSED;
				for (k = j + 1; k<nEdges0; k++) {
					pNode0->edges[k - 1] = pNode0->edges[k];
				}
				nEdges0--;
				pNode0->nEdges = nEdges0;
				memset(decendants, 0, sizeof(decendants));
				markDecendants(state, i, decendants);
				nEdges1 = pNode1->nEdges;
				for (k = 0; k<nEdges1; k++) {
					iNode2 = pNode1->edges[k];
					if (decendants[iNode2] == 0)
						pNode0->edges[nEdges0++] = iNode2;
				}
				pNode0->nEdges = nEdges0;
				j--;
				continue;
			}
		}
	}
}

void removeEdges(State *state)
{
	int i, j, k;
	int nNodes, nEdges;
	Node *pNode;
	int decendants[MAXNODE];


	nNodes = state->nNodes;
	for (i = 0; i<nNodes; i++) {
		pNode = &(state->nodes[i]);
		nEdges = pNode->nEdges;
		for (j = 0; j<nEdges; j++) {
			memset(decendants, 0, sizeof(decendants));
			for (k = 0; k<j; k++)
				markDecendants(state, pNode->edges[k], decendants);
			for (k = j + 1; k<nEdges; k++)
				markDecendants(state, pNode->edges[k], decendants);
			if (decendants[pNode->edges[j]]) {
				for (k = j + 1; k<nEdges - 1; k++)
					pNode->edges[k - 1] = pNode->edges[k];
				nEdges--;
				j--;
			}
		}
		pNode->nEdges = nEdges;
	}
}

void setInNodes(State *state)
{
	int i, j, nNodes, nEdges0;
	Node *pNode0;

	nNodes = state->nNodes;
	for (i = 0; i<nNodes; i++) {
		pNode0 = &(state->nodes[i]);
		if (pNode0->maxPath == UNUSED) continue;
		nEdges0 = pNode0->nEdges;
		for (j = 0; j<nEdges0; j++) {
			state->nodes[pNode0->edges[j]].inNodes++;
		}
	}
}

void mergeNodes(State *state)
{
	int i, j, nNodes, nEdges0, mask0;
	Node *pNode0, *pNode1;

	nNodes = state->nNodes;
	for (i = 1; i<nNodes; i++) {
		pNode0 = &(state->nodes[i]);
		if (pNode0->maxPath == UNUSED) continue;
		mask0 = pNode0->mask;
		while (pNode0->nEdges == 1) {
			pNode1 = &(state->nodes[pNode0->edges[0]]);
			if (pNode1->inNodes == 1 && pNode1->mask == mask0) {
				pNode1->maxPath = UNUSED;
				pNode0->nCards += pNode1->nCards;
				nEdges0 = pNode0->nEdges = pNode1->nEdges;
				for (j = 0; j<nEdges0; j++)
					pNode0->edges[j] = pNode1->edges[j];
				continue;
			}
			break;
		}
	}
}


/* maxPathを設定する */
/* hashValの計算をおこなう */

void traverse(State *state, int iNode0)
{
	int i, j, k;
	int mask0, path0;
	int nEdges0, nEdges1;
	int iNode1, iNode2;
	Node *pNode0, *pNode1;
	int hval0 = 0, hval1;

	pNode0 = &(state->nodes[iNode0]);
	nEdges0 = pNode0->nEdges;
	path0 = pNode0->maxPath;
	mask0 = pNode0->mask;
	hval0 = maskCount[mask0] + ((pNode0->nCards) << 2);
	for (i = 0; i<nEdges0; i++) {
		iNode1 = pNode0->edges[i];
		pNode1 = &(state->nodes[iNode1]);
		if (pNode1->maxPath<path0 + 1) {
			pNode1->maxPath = path0 + 1;
			traverse(state, iNode1);
		}
		hval1 = pNode1->hashVal;
		hval0 += (hval1 << 4) + hval1;
	}
	pNode0->hashVal = hval0;
}

void sortNodes(State *state, int *newIndex)
{
	int i, j, nNodes, flag;
	Node tmpNode;

	nNodes = state->nNodes;
	for (i = 0; i<nNodes; i++) {
		newIndex[i] = i;
	}
	for (i = 2; i<nNodes; i++) {
		if (cmpNode(&(state->nodes[i - 1]), &(state->nodes[i]))>0) {
			tmpNode = state->nodes[i];
			state->nodes[i] = state->nodes[i - 1];
			newIndex[state->nodes[i - 1].oldIndex] = i;
			for (j = i - 1; j >= 2; j--) {
				if (cmpNode(&(state->nodes[j - 1]), &tmpNode) <= 0) break;
				state->nodes[j] = state->nodes[j - 1];
				newIndex[state->nodes[j].oldIndex] = j;
			}
			state->nodes[j] = tmpNode;
			newIndex[tmpNode.oldIndex] = j;
		}
	}
	for (i = nNodes - 1; i >= 1;) {
		if (state->nodes[i].maxPath != UNUSED) {
			break;
		}
		i--;
	}
	nNodes = i + 1;
	state->nNodes = nNodes;
}

void sortEdges(State *state, int *newIndex)
{
	int i, j, k, tmp, tmp1, nNodes, nEdges0;
	Node *pNode0;
	int *pEdges0;

	nNodes = state->nNodes;
	for (i = 0; i<nNodes; i++) {
		pNode0 = &(state->nodes[i]);
		nEdges0 = pNode0->nEdges;
		pEdges0 = &(pNode0->edges[0]);
		for (j = 0; j<nEdges0; j++)
			pEdges0[j] = newIndex[pEdges0[j]];
		for (j = 1; j<nEdges0; j++) {
			if ((tmp1 = pEdges0[j - 1])>(tmp = pEdges0[j])) {
				pEdges0[j] = tmp1;
				for (k = j - 1; k >= 1; k--) {
					if ((tmp1 = pEdges0[k - 1]) <= tmp) break;
					pEdges0[k] = tmp1;
				}
				pEdges0[k] = tmp;
			}
		}
	}
}

void sortMasks(State *state)
{
	int i, j, k, nNodes;
	int mask_i, mask_j, mask_j1, tmpmask, tmpmask1;

	nNodes = state->nNodes;
	for (mask_i = 1, i = 1; i<NSTACK; i++, mask_i <<= 1) {
		mask_j1 = mask_i;
		mask_j = mask_j1 << 1;
		for (j = i; j >= 1; j--, mask_j = mask_j1, mask_j1 >>= 1) {
			for (k = 0; k<nNodes; k++) {
				tmpmask = state->nodes[k].mask;
				if (((tmpmask ^ (tmpmask << 1))&mask_j) == 0) continue;
				if ((tmpmask&mask_j) == 0) goto swap;
				else break;
			}
		noswap: continue;
		swap: {
			int mask_jj1, cmask_jj1, tmpmask2;

			mask_jj1 = mask_j | mask_j1;
			cmask_jj1 = ~mask_jj1;
			for (; k<nNodes; k++) {
				tmpmask = state->nodes[k].mask;
				tmpmask1 = tmpmask & cmask_jj1;
				tmpmask2 = tmpmask & mask_jj1;
				tmpmask1 = tmpmask1 | (((tmpmask2 >> 1) | (tmpmask2 << 1))&mask_jj1);
				state->nodes[k].mask = tmpmask1;
			}
		}
		}
	}
}

void initNormalize(State* state)
{
	int i, nNodes;
	Node *pNode0;

	nNodes = state->nNodes;
	for (i = 0; i<nNodes; i++) {
		pNode0 = &(state->nodes[i]);
		pNode0->maxPath = 0;
		pNode0->inNodes = 0;
		pNode0->hashVal = 0;
		pNode0->oldIndex = i;
	}
}

void normalizeState(State *state)
{
	int newIndex[MAXNODE];
	initNormalize(state);
	/* nCardsが0のNodeを除去する */
	removeZeroNodes(state);
	/* 必要かわからないが 不要の edgeを削除する */
	removeEdges(state);
	/* inNodesを set する． */
	setInNodes(state);
	/* merge 可能なノードを merge する */
	mergeNodes(state);
	/* maxPath, hashValの計算 */
	traverse(state, 0);
	/* Nodeのソート */
	sortNodes(state, newIndex);
	/* edge のソート */
	sortEdges(state, newIndex);
	/* mask のソート */
	sortMasks(state);
}
#else
/* サイズが0のノードは取り除くことができる */
/* maxPath を UNUSEDにすることが削除を意味する */

void sortEdges(State *state, int *newIndex);
void removeZeroNodes(State *state)
{
	int i, j, k, nNodes, index;
	Node *pNode0, *pNode1;
	int decendants[MAXNODE];
	int nEdges0, nEdges1;
	int iNode2;
	int newIndex[MAXNODE];
	int flag = 0;

	nNodes = state->nNodes;
	for (i = 0; i<nNodes; i++) {
		pNode0 = &(state->nodes[i]);
		nEdges0 = pNode0->nEdges;
		memset(decendants, 0, sizeof(int)*nNodes);
		for (j = 0; j<nEdges0; j++) {
			decendants[pNode0->edges[j]] = 1;
			pNode1 = &(state->nodes[pNode0->edges[j]]);
			if (pNode1->nCards == 0) {
				pNode1->maxPath = UNUSED;
				flag = 1;
				for (k = j + 1; k<nEdges0; k++) {
					pNode0->edges[k - 1] = pNode0->edges[k];
				}
				nEdges0--;
				pNode0->nEdges = nEdges0;
				nEdges1 = pNode1->nEdges;
				for (k = 0; k<nEdges1; k++) {
					iNode2 = pNode1->edges[k];
					if (!decendants[iNode2]) {
						decendants[iNode2] = 1;
						pNode0->edges[nEdges0++] = iNode2;
					}
				}
				pNode0->nEdges = nEdges0;
				j--;
				continue;
			}
		}
	}
	if (flag) {
		for (index = i = 0; i<nNodes; i++) {
			if (state->nodes[i].maxPath != UNUSED) {
				if (i != index) state->nodes[index] = state->nodes[i];
				newIndex[i] = index;
				index++;
			}
		}
		state->nNodes = index;
		sortEdges(state, newIndex);
	}
}

void setMaxPath(State *state, int iNode0)
{
	int i, j, k;
	int mask0, path0;
	int nEdges0, nEdges1;
	int iNode1, iNode2;
	Node *pNode0, *pNode1;
	int hval0 = 0, hval1;

	pNode0 = &(state->nodes[iNode0]);
	nEdges0 = pNode0->nEdges;
	path0 = pNode0->maxPath;
	for (i = 0; i<nEdges0; i++) {
		iNode1 = pNode0->edges[i];
		pNode1 = &(state->nodes[iNode1]);
		if (pNode1->maxPath<path0 + 1) {
			pNode1->maxPath = path0 + 1;
			setMaxPath(state, iNode1);
		}
	}
}

void sortEdges(State *state, int *newIndex);
void topologicalSort(State *state)
{
	int i, j, nNodes, tmpPath;
	Node tmpNode;
	int newIndex[MAXNODE];

	nNodes = state->nNodes;
	for (i = 0; i<nNodes; i++)
		state->nodes[i].oldIndex = i;
	setMaxPath(state, 0);
	for (i = 2; i<nNodes; i++) {
		if (state->nodes[i - 1].maxPath>(tmpPath = state->nodes[i].maxPath)) {
			tmpNode = state->nodes[i];
			state->nodes[i] = state->nodes[i - 1];
			for (j = i - 1; j >= 2; j--) {
				if (state->nodes[j - 1].maxPath <= tmpPath) break;
				state->nodes[j] = state->nodes[j - 1];
			}
			state->nodes[j] = tmpNode;
		}
	}
#if 0
	for (i = nNodes - 1; i >= 1;) {
		if (state->nodes[i].maxPath != UNUSED) {
			break;
		}
		i--;
	}
	nNodes = i + 1;
	state->nNodes = nNodes;
#endif
	for (i = 0; i<nNodes; i++)
		newIndex[state->nodes[i].oldIndex] = i;
	sortEdges(state, newIndex);
}

void removeEdges(State *state)
{
	int i, j, k, index, toChildren;
	int nNodes, nEdges;
	Node *pNode;
	int parents[MAXNODE], parent, edge;

	memset(parents, 0, sizeof(parents));
	nNodes = state->nNodes;
	for (i = 0; i<nNodes; i++) {
		toChildren = parents[i] | (1 << i);
		parents[i] = toChildren;
		pNode = &(state->nodes[i]);
		nEdges = pNode->nEdges;
		for (j = 0; j<nEdges; j++)
			parents[pNode->edges[j]] |= toChildren;
	}
	for (i = 0; i<nNodes; i++) {
		pNode = &(state->nodes[i]);
		nEdges = pNode->nEdges;
		for (j = index = 0; j<nEdges; j++) {
			parent = parents[pNode->edges[j]];
			for (k = 0; k<index; k++) {
				edge = pNode->edges[k];
				if ((parent >> edge) & 1) goto noedge;
			}
			if (index != j) pNode->edges[index] = pNode->edges[j];
			index++;
		noedge:;
		}
		pNode->nEdges = index;
	}
}

void setInNodes(State *state)
{
	int i, j, nNodes, nEdges0;
	Node *pNode0;

	nNodes = state->nNodes;
	for (i = 0; i<nNodes; i++) {
		pNode0 = &(state->nodes[i]);
#if 0
		if (pNode0->maxPath == UNUSED) continue;
#endif
		nEdges0 = pNode0->nEdges;
		for (j = 0; j<nEdges0; j++) {
			state->nodes[pNode0->edges[j]].inNodes++;
		}
	}
}

void mergeNodes(State *state)
{
	int i, j, nNodes, nEdges0, mask0;
	Node *pNode0, *pNode1;
	int index, newIndex[MAXNODE];
	int mflag = 0;

	nNodes = state->nNodes;
	for (i = 1; i<nNodes; i++) {
		pNode0 = &(state->nodes[i]);
#if 0
		if (pNode0->maxPath == UNUSED) continue;
#endif
		mask0 = pNode0->mask;
		while (pNode0->nEdges == 1) {
			pNode1 = &(state->nodes[pNode0->edges[0]]);
			if (pNode1->inNodes == 1 && pNode1->mask == mask0) {
				pNode1->maxPath = UNUSED;
				mflag = 1;
				pNode0->nCards += pNode1->nCards;
				nEdges0 = pNode0->nEdges = pNode1->nEdges;
				for (j = 0; j<nEdges0; j++)
					pNode0->edges[j] = pNode1->edges[j];
				continue;
			}
			break;
		}
	}
	if (mflag) {
		for (index = i = 0; i<nNodes; i++) {
			if (state->nodes[i].maxPath != UNUSED) {
				if (i != index) state->nodes[index] = state->nodes[i];
				newIndex[i] = index;
				index++;
			}
		}
		state->nNodes = index;
		sortEdges(state, newIndex);
		{
			int i, j, nNodes = state->nNodes, nEdges, n, mp0;
			Node *pNode;
			for (i = 0; i<nNodes; i++) {
				state->nodes[i].maxPath = 0;
			}
			for (i = 0; i<nNodes; i++) {
				pNode = &(state->nodes[i]);
				mp0 = pNode->maxPath;
				nEdges = pNode->nEdges;
				for (j = 0; j<nEdges; j++) {
					n = pNode->edges[j];
					if (state->nodes[n].maxPath<mp0 + 1)
						state->nodes[n].maxPath = mp0 + 1;
				}
			}
		}
	}
}


/* maxPathを設定する */
/* hashValの計算をおこなう */

void traverse(State *state, int iNode0)
{
	int i, j, k;
	int mask0, path0;
	int nEdges0, nEdges1;
	int iNode1, iNode2;
	Node *pNode0, *pNode1;
	int hval0 = 0, hval1;

	pNode0 = &(state->nodes[iNode0]);
#if 0
	if (pNode0->maxPath == UNUSED) {
		abort();
	}
#endif
	nEdges0 = pNode0->nEdges;
	path0 = pNode0->maxPath;
	mask0 = pNode0->mask;
#if 0
	if (path0>UNUSED) {
		abort();
	}
#endif
	hval0 = maskCount[mask0] + ((pNode0->nCards) << 2);
	for (i = 0; i<nEdges0; i++) {
		iNode1 = pNode0->edges[i];
		pNode1 = &(state->nodes[iNode1]);
		if (pNode1->maxPath<path0 + 1) {
			pNode1->maxPath = path0 + 1;
			traverse(state, iNode1);
		}
		hval1 = pNode1->hashVal;
		hval0 += (hval1 << 4) + hval1;
	}
	pNode0->hashVal = hval0;
}

void setHval(State *state)
{
	int i, nNodes, j, nEdges0, mask0, hval0, iNode1, hval1;
	Node *pNode0, *pNode1;

	nNodes = state->nNodes;
	for (j = nNodes - 1; j >= 1; j--) {
		pNode0 = &(state->nodes[j]);
		nEdges0 = pNode0->nEdges;
		mask0 = pNode0->mask;
		hval0 = maskCount[mask0] + ((pNode0->nCards) << 2);
		for (i = 0; i<nEdges0; i++) {
			iNode1 = pNode0->edges[i];
			pNode1 = &(state->nodes[iNode1]);
			hval1 = pNode1->hashVal;
			hval0 += (hval1 << 4) + hval1;
		}
		pNode0->hashVal = hval0;
	}
}

void sortNodes(State *state, int *newIndex)
{
	int i, j, nNodes, flag;
	Node tmpNode;

	nNodes = state->nNodes;
	for (i = 0; i<nNodes; i++) state->nodes[i].oldIndex = i;
	for (i = 2; i<nNodes; i++) {
		if (cmpNode(&(state->nodes[i - 1]), &(state->nodes[i]))>0) {
			tmpNode = state->nodes[i];
			state->nodes[i] = state->nodes[i - 1];
			for (j = i - 1; j >= 2; j--) {
				if (cmpNode(&(state->nodes[j - 1]), &tmpNode) <= 0) break;
				state->nodes[j] = state->nodes[j - 1];
			}
			state->nodes[j] = tmpNode;
		}
	}
#if 0
	for (i = nNodes - 1; i >= 1;) {
		if (state->nodes[i].maxPath != UNUSED) {
			break;
		}
		i--;
	}
	nNodes = i + 1;
	state->nNodes = nNodes;
#endif
	for (i = 0; i<nNodes; i++)
		newIndex[state->nodes[i].oldIndex] = i;
}

void sortEdges(State *state, int *newIndex)
{
	int i, j, k, tmp, tmp1, nNodes, nEdges0;
	Node *pNode0;
	int *pEdges0;

	nNodes = state->nNodes;
	for (i = 0; i<nNodes; i++) {
		pNode0 = &(state->nodes[i]);
		nEdges0 = pNode0->nEdges;
		pEdges0 = &(pNode0->edges[0]);
		for (j = 0; j<nEdges0; j++)
			pEdges0[j] = newIndex[pEdges0[j]];
		for (j = 1; j<nEdges0; j++) {
			if ((tmp1 = pEdges0[j - 1])>(tmp = pEdges0[j])) {
				pEdges0[j] = tmp1;
				for (k = j - 1; k >= 1; k--) {
					if ((tmp1 = pEdges0[k - 1]) <= tmp) break;
					pEdges0[k] = tmp1;
				}
				pEdges0[k] = tmp;
			}
		}
	}
}

void sortMasks(State *state)
{
	int i, j, k, nNodes;
	int mask_i, mask_j, mask_j1, tmpmask, tmpmask1;

	nNodes = state->nNodes;
	for (mask_i = 1, i = 1; i<NSTACK; i++, mask_i <<= 1) {
		mask_j1 = mask_i;
		mask_j = mask_j1 << 1;
		for (j = i; j >= 1; j--, mask_j = mask_j1, mask_j1 >>= 1) {
			for (k = 0; k<nNodes; k++) {
				tmpmask = state->nodes[k].mask;
				if (((tmpmask ^ (tmpmask << 1))&mask_j) == 0) continue;
				if ((tmpmask&mask_j) == 0) goto swap;
				else break;
			}
		noswap: continue;
		swap: {
			int mask_jj1, cmask_jj1, tmpmask2;

			mask_jj1 = mask_j | mask_j1;
			cmask_jj1 = ~mask_jj1;
			for (; k<nNodes; k++) {
				tmpmask = state->nodes[k].mask;
				tmpmask1 = tmpmask & cmask_jj1;
				tmpmask2 = tmpmask & mask_jj1;
				tmpmask1 = tmpmask1 | (((tmpmask2 >> 1) | (tmpmask2 << 1))&mask_jj1);
				state->nodes[k].mask = tmpmask1;
			}
		}
		}
	}
}

void initNormalize(State* state)
{
	int i, nNodes;
	Node *pNode0;

	nNodes = state->nNodes;
	for (i = 0; i<nNodes; i++) {
		pNode0 = &(state->nodes[i]);
		pNode0->maxPath = 0;
		pNode0->inNodes = 0;
#if 0
		pNode0->hashVal = 0;
		pNode0->oldIndex = i;
#endif
	}
}

void normalizeState(State *state)
{
	int newIndex[MAXNODE];
	initNormalize(state);
	/* nCardsが0のNodeを除去する */
	removeZeroNodes(state);
	/* 必要かわからないが 不要の edgeを削除する */
	topologicalSort(state);
	removeEdges(state);
	/* inNodesを set する． */
	setInNodes(state);
	/* merge 可能なノードを merge する */
	mergeNodes(state);
	/* maxPath, hashValの計算 */
#if 0
	traverse(state, 0);
#else
	setHval(state);
#endif
	/* Nodeのソート */
	sortNodes(state, newIndex);
	/* edge のソート */
	sortEdges(state, newIndex);
	/* mask のソート */
	sortMasks(state);
	int i;
	// showState(state);
	// cout << "at normalizwend"; cin >> i;
}
#endif

/* Huffman Encoding */

/* 先頭に長さを持つ */
/* mask, n */
/* nEdges */
/* edgeはlastEdgeとの差 */

#define BUFSIZE 16 /* どんなコードも長さが16を超えない */

typedef struct _HuffmanState {
	unsigned int buf;
	int bufLen;
	unsigned char *ptr;
} HuffmanState;

typedef struct _HuffmanTable {
	unsigned int bitPattern;
	int bitLen;
	int val;
} HuffmanTable;

void initHuffmanState(HuffmanState *h, unsigned char *ptr)
{
	h->buf = 0;
	h->bufLen = 0;
	h->ptr = ptr;
}

int decodeHuffman(HuffmanState *hs, const HuffmanTable *ht, int len)
{
	int i;
	while (hs->bufLen<BUFSIZE) {
		hs->buf |= ((*(hs->ptr)++) << (24 - hs->bufLen));
		hs->bufLen += 8;
	}
	for (i = 0; i<len - 1; i++)
		if (ht[i + 1].bitPattern>hs->buf) break;
	hs->buf <<= ht[i].bitLen;
	hs->bufLen -= ht[i].bitLen;
	return ht[i].val;
}

void encodeHuffman(HuffmanState *hs, const HuffmanTable *ht, int len, int val)
{
	int i;
	for (i = 0; i<len; i++)
		if (ht[i].val == val) break;
	if (i == len) { /* encode すべきコードがみつからない場合 */
		abort();
	}
	hs->buf |= ht[i].bitPattern >> hs->bufLen;
	hs->bufLen += ht[i].bitLen;
	while (hs->bufLen >= 8) {
		*(hs->ptr)++ = (hs->buf >> 24) & 255;
		hs->buf <<= 8;
		hs->bufLen -= 8;
	}
}

void flushHuffman(HuffmanState *hs)
{
	while (hs->bufLen>0) {
		*(hs->ptr)++ = (hs->buf >> 24) & 255;
		hs->bufLen -= 8;
	}
}

#if NSTACK==4
const HuffmanTable maskTable[] = {
	{ 0x00000000,2,0 },
{ 0x40000000,2,8 },
{ 0x80000000,2,12 },
{ 0xc0000000,3,14 },
{ 0xe0000000,5,4 },
{ 0xe8000000,5,10 },
{ 0xf0000000,5,13 },
{ 0xf8000000,7,6 },
{ 0xfa000000,7,2 },
{ 0xfc000000,7,11 },
{ 0xfe000000,8,9 },
{ 0xff000000,10,7 },
{ 0xff400000,10,3 },
{ 0xff800000,10,1 },
{ 0xffc00000,10,5 },
};
#else /* NSTACK==3 */
const HuffmanTable maskTable[] = {
	{ 0x00000000,2,0 },
{ 0x40000000,2,4 },
{ 0x80000000,2,6 },
{ 0xc0000000,3,7 },
{ 0xe0000000,5,2 },
{ 0xe8000000,5,5 },
{ 0xf0000000,5,3 },
{ 0xf8000000,5,1 },
};
#endif

#define DECODE_MASK(hs) \
  decodeHuffman(hs,maskTable,sizeof(maskTable)/sizeof(HuffmanTable))
#define ENCODE_MASK(hs,val) \
  encodeHuffman(hs,maskTable,sizeof(maskTable)/sizeof(HuffmanTable),val)

const HuffmanTable nCardsTable[] = {
	{ 0x00000000,1,1 },
{ 0x80000000,2,2 },
{ 0xc0000000,3,3 },
{ 0xe0000000,4,4 },
{ 0xf0000000,5,5 },
{ 0xf8000000,6,6 },
{ 0xfc000000,7,7 },
{ 0xfe000000,8,8 },
{ 0xff000000,16,9 },
{ 0xff010000,16,10 },
{ 0xff020000,16,11 },
{ 0xff030000,16,12 },
{ 0xff040000,16,13 },
{ 0xff050000,16,14 },
{ 0xff060000,16,15 },
{ 0xff070000,16,16 },
{ 0xff080000,16,17 },
{ 0xff090000,16,18 },
{ 0xff0a0000,16,19 },
{ 0xff0b0000,16,20 },
{ 0xff0c0000,16,21 },
{ 0xff0d0000,16,22 },
{ 0xff0e0000,16,23 },
{ 0xff0f0000,16,24 },
{ 0xff100000,16,25 },
{ 0xff110000,16,26 },
{ 0xff120000,16,27 },
{ 0xff130000,16,28 },
{ 0xff140000,16,29 },
{ 0xff150000,16,30 },
{ 0xff160000,16,31 },
};
#define DECODE_NCARDS(hs) \
  decodeHuffman(hs,nCardsTable,sizeof(nCardsTable)/sizeof(HuffmanTable))
#define ENCODE_NCARDS(hs,val) \
  encodeHuffman(hs,nCardsTable,sizeof(nCardsTable)/sizeof(HuffmanTable),val)


const HuffmanTable nEdgesTable[] = {
	{ 0x00000000,1,2 },
{ 0x80000000,2,1 },
{ 0xc0000000,2,0 },
};

#define DECODE_NEDGES(hs) \
  decodeHuffman(hs,nEdgesTable,sizeof(nEdgesTable)/sizeof(HuffmanTable))
#define ENCODE_NEDGES(hs,val) \
  encodeHuffman(hs,nEdgesTable,sizeof(nEdgesTable)/sizeof(HuffmanTable),val)

const HuffmanTable edgeTable[] = {
	{ 0x00000000,1,1 },
{ 0x80000000,2,2 },
{ 0xc0000000,3,3 },
{ 0xe0000000,4,4 },
{ 0xf0000000,5,5 },
{ 0xf8000000,6,6 },
{ 0xfc000000,7,7 },
{ 0xfe000000,8,8 },
{ 0xff000000,9,9 },
{ 0xff800000,10,10 },
{ 0xffc00000,10,11 }
};

#define DECODE_EDGE(hs) \
  decodeHuffman(hs,edgeTable,sizeof(edgeTable)/sizeof(HuffmanTable))
#define ENCODE_EDGE(hs,val) \
  encodeHuffman(hs,edgeTable,sizeof(edgeTable)/sizeof(HuffmanTable),val)

#define MAXPACKED 255

int packState(unsigned char *packed, State *state)
{
	int nNodes, nEdges, all_edges, size;
	int i, j, lastEdge;
	unsigned char *ptr;
	Node *pNode0;
	int *pEdges;
	HuffmanState hs;

	nNodes = state->nNodes;
	initHuffmanState(&hs, packed);
	for (i = 0; i<nNodes; i++) {
		pNode0 = &(state->nodes[i]);
		if (i != 0) {
			ENCODE_NCARDS(&hs, pNode0->nCards);
		}
		ENCODE_MASK(&hs, pNode0->mask);
		nEdges = pNode0->nEdges;
		ENCODE_NEDGES(&hs, nEdges);
		pEdges = &(pNode0->edges[0]);
		lastEdge = i;
		for (j = 0; j<nEdges; j++) {
			ENCODE_EDGE(&hs, pEdges[j] - lastEdge);
			lastEdge = pEdges[j];
		}
	}
	flushHuffman(&hs);
	return (hs.ptr) - packed;
}

int unpackState(State *state, unsigned char *ptr)
{
	HuffmanState hs;
	int maxNode = 0;
	int i, j;
	int nEdges, edge, lastEdge;

	initHuffmanState(&hs, ptr);
	for (i = 0; i <= maxNode; i++) {
		if (i == 0) {
			state->nodes[i].nCards = 0;
		}
		else {
			state->nodes[i].nCards = DECODE_NCARDS(&hs);
		}
		state->nodes[i].mask = DECODE_MASK(&hs);
		nEdges = state->nodes[i].nEdges = DECODE_NEDGES(&hs);
		lastEdge = i;
		for (j = 0; j<nEdges; j++) {
			state->nodes[i].edges[j] = edge = DECODE_EDGE(&hs) + lastEdge;
			if (edge>maxNode)maxNode = edge;
			lastEdge = edge;
		}
	}
	state->nNodes = i;
	return hs.ptr - ptr - (hs.bufLen / 8);
}

const int perm4[24][4] = {
	{ 0,1,2,3 },
{ 1,0,2,3 },
{ 0,2,1,3 },
{ 1,2,0,3 },
{ 2,0,1,3 },
{ 2,1,0,3 },
{ 0,1,3,2 },
{ 0,2,3,1 },
{ 0,3,1,2 },
{ 0,3,2,1 },
{ 1,0,3,2 },
{ 1,2,3,0 },
{ 1,3,0,2 },
{ 1,3,2,0 },
{ 2,0,3,1 },
{ 2,1,3,0 },
{ 2,3,0,1 },
{ 2,3,1,0 },
{ 3,0,1,2 },
{ 3,0,2,1 },
{ 3,1,0,2 },
{ 3,1,2,0 },
{ 3,2,0,1 },
{ 3,2,1,0 },
};

const int permlen[] = { 0,1,2,6,24 };

int findBestNorm(State *state,
	int nstarts, int *starts, int *sizes,
	unsigned char *best_packed, int rsize)
{
	int i, j, k, l;
	State tmpstate;
	int nNodes;
	Node *pNode0;
	int nEdges0;
	unsigned char packed[MAXPACKED];
	int size, len, start;
	int newIndex[MAXNODE];
	int packed_size;

	if (nstarts == 0) {
		for (i = 0; i<MAXNODE; i++)
			newIndex[i] = i;
		sortEdges(state, newIndex);
		sortMasks(state);
		packed_size = packState(packed, state);
		if (packed_size <= rsize)
			if (packed_size<rsize || cmp_packed(packed, best_packed, packed_size)>0) {
				memcpy(best_packed, packed, MAXPACKED);
				rsize = packed_size;
			}
		return rsize;
	}
	nstarts--;
	size = sizes[nstarts];
	if (size>4) {
		abort();
	}
	nNodes = state->nNodes;
	len = permlen[size];
	start = starts[nstarts];
	for (i = 0; i<len; i++) {
		copyState(&tmpstate, state);
		for (j = 0; j<size; j++) {
			tmpstate.nodes[start + perm4[i][j]] = state->nodes[start + j];
		}
		for (j = 0; j<nNodes; j++) {
			pNode0 = &(tmpstate.nodes[j]);
			nEdges0 = pNode0->nEdges;
			for (k = 0; k<nEdges0; k++) {
				l = pNode0->edges[k] - start;
				if (0 <= l && l<size)
					pNode0->edges[k] = start + perm4[i][l];
			}
		}
		rsize = findBestNorm(&tmpstate, nstarts, starts, sizes, best_packed, rsize);
	}
	return rsize;
}



/* 完全に normalize する */

int normPackState(unsigned char *packed, State *state)
{
	int i, nNodes, start;
	int starts[MAXNODE];
	int sizes[MAXNODE];
	int nstarts = 0;

	nNodes = state->nNodes;
	for (i = 1; i<nNodes - 1; i++) {
		if (cmpNode(&(state->nodes[i]), &(state->nodes[i + 1])) == 0) {
			starts[nstarts] = start = i++;
			while (i<nNodes - 1 &&
				cmpNode(&(state->nodes[start]), &(state->nodes[i + 1])) == 0) i++;
			sizes[nstarts++] = i - start + 1;
		}
	}
	if (nstarts == 0) {
		int packed_size = packState(packed, state);
		return packed_size;
	}
	return findBestNorm(state, nstarts, starts, sizes, packed, 255);
}

int allcount = 0;

const int hashsize = 8 * 1024 * 1024;

typedef struct _PackedState {
	Float prob;
	struct _PackedState *next;
	unsigned char packed[1];
} PackedState;

#ifdef WALIGN
static PackedState *getNext(PackedState *ptr)
{
	PackedState *tmp;
	memcpy(&tmp, &ptr->next, sizeof(PackedState*));
	return tmp;
}
static void setNext(PackedState *var, PackedState *val)
{
	memcpy(&var->next, &val, sizeof(PackedState*));
}
static Float getProb(PackedState *ptr)
{
	Float prob;
	memcpy(&prob, &ptr->prob, sizeof(Float));
	return prob;
}
static void setProb(PackedState *var, Float prob)
{
	Float tmp = prob;
	memcpy(&var->prob, &tmp, sizeof(Float));
}

#else
#define getNext(ptr) (ptr->next)
#define setNext(var,val) (var->next=val)
#define getProb(ptr) (ptr->prob)
#define setProb(var,val) (var->prob=val)
#endif

unsigned int *hashtable;

unsigned int hash_packed(unsigned char *ptr, int size)
{
	int i;
	unsigned int r = 0;

	for (i = size - 1; i >= 0; i--) {
		r = (r << 8) + (r >> 8);
		r += *(ptr + i);
	}
	return r % (hashsize*NPROCS);
}

#define LOCAL_HASH(hval) ((hval)%hashsize)
#define PROC_HASH(hval)  ((hval)/hashsize)

int cmp_packed(unsigned char *ptr0, unsigned char *ptr1, int size)
{
	return memcmp(ptr0, ptr1, size);
}

#define NDIV 8
int datfd;
int datfds[NDIV];
void initDatFile(const char *filename)
{
	int headerSize, i;
	char tmpfilename[256];
	int rsize;


	if ((hashtable = (unsigned int*)malloc(hashsize * sizeof(unsigned int))) == NULL) {
		fprintf(stderr, "Failed malloc\n");
		exit(1);
	}

	if ((datfd = open(filename, O_RDONLY | O_BINARY))<0) {
		fprintf(stderr, "Filed opening file %s\n", filename);
		exit(1);
	}

	for (i = 0; i<NDIV; i++) {
		sprintf(tmpfilename, "%s.%d", filename, i);
		//	printf("NVIV file=%s\n",tmpfilename);
		if ((datfds[i] = _open(tmpfilename, O_RDONLY | O_BINARY))<0) {
			fprintf(stderr, "Filed opening file %s\n", tmpfilename);
			exit(1);
		}
	}

	headerSize = lseek(datfd, 0, 2);
	unsigned char *p = (unsigned char *)hashtable;

	fprintf(stderr, "#%08x\n", hashsize);//OK
										 //  hashtable= new (unsigned int*) [hashsize-1];
										 //lseek(datfd,hashsize*sizeof(unsigned int),SEEK_SET);
	_lseek(datfd, 0, SEEK_SET);

#if NSTACK==4 //AAAAA
	lseek(datfd, hashsize * sizeof(unsigned int), SEEK_SET);
#endif
	read(datfd, hashtable, hashsize * sizeof(unsigned int));
	
#if NSTACK==4 //AAAAA
	{
		int i, j;
		int tmp;
		unsigned char *tmpp;

		tmpp = (unsigned char *)&tmp;
		for (i = 0; i<hashsize; i++) {
			for (j = 0; j<4; j++)
				tmpp[j] = *((char *)hashtable + i * 4 + 3 - j);
			hashtable[i] = tmp;
		}
	}
#endif  

}
int ghval;
double findPacked(unsigned char *ptr, int size, unsigned char *packed, int packedSize)
{
	int offset = 0;
	int packedSize1;
	Float prob;

	while (offset + 1 + sizeof(Float)<size) {
		packedSize1 = ptr[offset] & 255;
		if (packedSize1 == packedSize && cmp_packed(&ptr[offset + sizeof(Float) + 1], packed, packedSize) == 0) {
//#ifdef LITTLE_ENDIAN
#if NSTACK==4 //AAAAA

			{
				int i;
				for (i = 0; i<sizeof(Float); i++)
					*((char *)(&prob) + i) = ptr[offset + sizeof(Float) - i - 1 + 1];
			}
#else
			memcpy(&prob, &ptr[offset + 1], sizeof(Float));
#endif
			return prob;
		}
		offset += packedSize1 + sizeof(Float) + 1;
		;
	}
	if (debuglevel < 4) return 0.0;
	{
		int i, j;
		State tmpstate;
		unpackState(&tmpstate, packed);
		printf("Can't find state\n");
		showState(&tmpstate);
		return 0.0;
	}
}

#define BLOCKSIZE 1


double stateProb(State *state)
{
	unsigned char packed[1024];
	// unsigned char tmpbuf[1024*1024];
#define temps 10000
	unsigned char tmpbuf[temps];
	int headerSize, packedSize, hval;
	int size;
	double prob;
	int div;
	int divsize;

	if (state->nNodes == 1) return 1.0;
	packedSize = normPackState(packed, state);
	hval = hash_packed(packed, packedSize);
	ghval = hval;
	divsize = hashsize / NDIV;
	if (hval%divsize == divsize - 1) size = temps;
	else
		size = (hashtable[hval + 1] - hashtable[hval])*BLOCKSIZE;
	// printf("reqsize=%d hval=%d\n",size,hval);
	headerSize = hashsize * sizeof(unsigned int);
	div = hval / (hashsize / NDIV);
	_lseek(datfds[div], (off_t)hashtable[hval] * BLOCKSIZE, SEEK_SET);
	size = _read(datfds[div], tmpbuf, size);
	// printf("read div=%d  size=%d offset=%d BS=%d \n",div,size,hashtable[hval],BLOCKSIZE);
	prob = findPacked(tmpbuf, size, packed, packedSize);
	//  assert(size);
	//  cout << "stateprob " << prob << endl;
	//  cout << "packedsize=" << packedSize << " no_nofe=" << state->nNodes <<	  " stateprob " << prob << endl;
	//  if (packedSize>9)cin >> div;
	return prob;
}


#define NOCARD -1

typedef struct _Stack {
	int nCards; /* いくつのカードが積まれているか? */
	int cards[MAXCARD]; /* カードの番号 */
} Stack;

void pushStack(Stack *stack, int num)
{
	stack->cards[stack->nCards++] = num;
}

int popStack(Stack *stack)
{
	if (stack->nCards == 0) {
		abort();
	}
	return stack->cards[--stack->nCards];
}

int topOfStack(Stack *stack)
{
	if (stack->nCards == 0) return NOCARD;
	return stack->cards[stack->nCards - 1];
}

typedef struct _PlayState {
	Stack stacks[NSTACK];
	int deck[NDECKS];
	int nCards; /* すでに何枚カードが出たか */
	int cards[MAXCARD]; /* それぞれの内容は */
	int cardHist[13]; /* 後, 何枚カードが残っているか */
} PlayState;

void initPlayState(PlayState *playState)
{
	int i;

	playState->nCards = 0;
	for (i = 0; i<13; i++)
		playState->cardHist[i] = 4;
	for (i = 0; i<NSTACK; i++)
		playState->stacks[i].nCards = 0;
	for (i = 0; i<NDECKS; i++)
		playState->deck[i] = 0;
}

void showPlayState(PlayState *p)
{
	int i, j;

	for (i = 0; i<4; i++) {
		printf("%1d: ", i);
		for (j = 0; j<p->deck[i]; j++)
			printf("  ");
		for (; j<13; j++)
			printf("%c ", cardsym[12 - ((i + 1)*(12 - j)) % 13]);
		putchar('\n');
	}
	for (i = 0; i<NSTACK; i++) {
		printf("%1d: ", i);
		for (j = 0; j<p->stacks[i].nCards; j++)
			printf("%c ", cardsym[p->cards[p->stacks[i].cards[j]]]);
		putchar('\n');
	}
	for (i = 0; i<13; i++) {
		printf("  %c", cardsym[i]);
	}
	putchar('\n');
	for (i = 0; i<13; i++) {
		printf("  %d", p->cardHist[i]);
	}
	putchar('\n');
}

int cmpPlayState(PlayState *p0, PlayState *p1)
{
	int i, j;

	if (p0->nCards<p1->nCards) return -1;
	if (p0->nCards>p1->nCards) return 1;
	for (i = 0; i<NDECKS; i++) {
		if (p0->deck[i]<p1->deck[i]) return -1;
		if (p0->deck[i]>p1->deck[i]) return 1;
	}
	for (i = 0; i<NSTACK; i++) {
		if (p0->stacks[i].nCards<p1->stacks[i].nCards) return -1;
		if (p0->stacks[i].nCards>p1->stacks[i].nCards) return 1;
		for (j = p0->stacks[i].nCards - 1; j >= 0; j--) {
			if (p0->stacks[i].cards[j]<p1->stacks[i].cards[j]) return -1;
			if (p0->stacks[i].cards[j]>p1->stacks[i].cards[j]) return 1;
		}
	}
	for (i = p0->nCards - 1; i >= 0; i--) {
		if (p0->cards[i]<p1->cards[i]) return -1;
		if (p0->cards[i]>p1->cards[i]) return 1;
	}
	return 0;
}

typedef struct _HashPS {
	double prob;
	struct _HashPS *next;
	PlayState state;
} HashPS;

const int hashPSsize = 16 * 1024;
HashPS **hashPStable;

void initHashPS()
{
	if ((hashPStable = (HashPS **)malloc(hashPSsize * sizeof(HashPS *))) == NULL) {
		abort();
	}
	memset(hashPStable, 0, hashPSsize * sizeof(HashPS *));
}

void clearHashPS()
{
	int i;
	HashPS *ptr, *next;
	for (i = 0; i<hashPSsize; i++) {
		for (ptr = hashPStable[i]; ptr != NULL; ptr = next) {
			next = ptr->next;
			free(ptr);
		}
		hashPStable[i] = NULL;
	}
}

int hashPlayState(PlayState *p0)
{
	unsigned int r = p0->nCards;
	int i, j;

	for (i = 0; i<NDECKS; i++) {
		r = (r << 2) + r;
		r += p0->deck[i];
	}
	for (i = 0; i<NSTACK; i++) {
		r = (r << 2) + r;
		r += p0->stacks[i].nCards;
		for (j = p0->stacks[i].nCards - 1; j >= 0; j--) {
			r = (r << 2) + r;
			r += p0->stacks[i].cards[j];
		}
	}
	for (i = p0->nCards - 1; i >= 0; i--) {
		r = (r << 2) + r;
		r += p0->cards[i];
	}
	return r % hashPSsize;
}

HashPS *findHashPS(PlayState *p0)
{
	int hval = hashPlayState(p0);
	HashPS *ptr = hashPStable[hval];
	while (ptr != NULL) {
		if (cmpPlayState(p0, &(ptr->state)) == 0) return ptr;
		ptr = ptr->next;
	}
	return NULL;
}

void addHashPS(PlayState *p0, double prob)
{
	int hval = hashPlayState(p0);
	HashPS *ptr;

	if ((ptr = (HashPS *)malloc(sizeof(HashPS))) == NULL) {
		abort();
	}
	ptr->state = *p0;
	ptr->prob = prob;
	ptr->next = hashPStable[hval];
	hashPStable[hval] = ptr;
}

/* 割り当て案 */
/* plan は, cards をどの台に将来置くかによって決める */
/* したがって, 長さ MAXCARD の配列 */
/* PLAN をいくつ保持するかは MAXPLAN で決める */
#define MAXPLAN 1000 /* 200 */
int maxplan = 50;
#define DONE -1

typedef struct _Plan {
	double prob;
#if 1
	int maxIndex;
#endif
	int decks[MAXCARD];
} Plan;

#define copyPlan(p0,p1,nCards) \
  memcpy(p0,p1,sizeof(Plan)-(MAXCARD-(nCards))*sizeof(int))

void showPlayStatePlan(PlayState *p, Plan *plan)
{
	int i, j, deck;
	char ww[90];
#if 0
	for (i = 0; i<p->nCards; i++)
		printf("%c-%d ", cardsym[p->cards[i]], plan->decks[i]);
	putchar('\n');
#endif
	for (i = 0; i<4; i++) {
		printf("%1d: ", i);
		for (j = 0; j<p->deck[i]; j++)
			printf("  ");
		for (; j<13; j++)
			printf("%c ", cardsym[12 - ((i + 1)*(12 - j)) % 13]);
		putchar('\n');
	}

	for (i = 0; i<NSTACK; i++) {
		printf("%1d: ", i);
		for (j = 0; j<p->stacks[i].nCards; j++) {
			deck = plan->decks[p->stacks[i].cards[j]];
			if (deck >= 0)
				printf("%c-%d ", cardsym[p->cards[p->stacks[i].cards[j]]], deck);
			else
				printf("%c-? ", cardsym[p->cards[p->stacks[i].cards[j]]]);
		}
		putchar('\n');
	}
	for (i = 0; i<13; i++) {
		printf("  %c", cardsym[i]);
	}
	putchar('\n');
	for (i = 0; i<13; i++) {
		printf("  %d", p->cardHist[i]);
	}
	putchar('\n');
}

typedef struct _Plans {
	int nPlans;
#if 1
	int undefIndex;
#endif
	Plan plans[MAXPLAN];
} Plans;

void copyPlans(Plans *p0, Plans *p1, int nCards)
{
	int nPlans = p1->nPlans;
	int i;

#if 1
	p0->undefIndex = p1->undefIndex;
#endif
	p0->nPlans = nPlans;
	for (i = 0; i<nPlans; i++)
		memcpy(&(p0->plans[i]), &(p1->plans[i]), sizeof(Plan) - (MAXCARD - nCards) * sizeof(int));
}

void showPlayStatePlans(PlayState *p, Plans *plans)
{
	int i, j, k, deck, card, undefIndex = plans->undefIndex;
	char www[80];
	for (i = 0; i<4; i++) {
		fprintf(stderr, "#%1d: ", i + 1);
		fprintf(file, "%1d: ", i + 1);
		for (j = 0; j<p->deck[i]; j++)
		{
			fprintf(stderr, "  ");
			fprintf(file, "  ");
		}
		for (; j<13; j++)
		{
			fprintf(stderr, "%c ", cardsym[12 - ((i + 1)*(12 - j)) % 13]);
			fprintf(file, "%c ", cardsym[12 - ((i + 1)*(12 - j)) % 13]);
		}
		fprintf(file, "\n");
		fprintf(stderr, "\n");

	}

	if (debuglevel>6) {
		for (i = 0; i<13; i++) {
			fprintf(stderr, "  %c", cardsym[i]);
		}
		fprintf(stderr, "\n");

		for (i = 0; i<13; i++) {
			fprintf(stderr, "  %d", p->cardHist[i]);
		}
		fprintf(stderr, "\n");

	}
	for (k = 0; k<plans->nPlans; k++) {
		if (debuglevel>0) {
			fprintf(stderr, "#--------------------------------------------------\n");
			fprintf(stderr, "#prob=%e\n", plans->plans[k].prob);
		}
#if 0
		for (i = 0; i<p->nCards; i++)
			printf("%c(%d) ", cardsym[p->cards[i]], plans->plans[k].decks[i]);
		putchar('\n');
#endif
		for (i = 0; i<NSTACK; i++) {
			fprintf(stderr, "#%1d: ", i + 1);  fprintf(file, "#%1d: ", i + 1);
			for (j = 0; j<p->stacks[i].nCards; j++) {
				card = p->stacks[i].cards[j];
				//	printf(" card val=%d ",card);
				deck = plans->plans[k].decks[card];
				if (card != undefIndex) {
					fprintf(stderr, "%c-%d ", cardsym[p->cards[p->stacks[i].cards[j]]], deck + 1);
					fprintf(file, "%c-%d ", cardsym[p->cards[p->stacks[i].cards[j]]], deck + 1);
				}
				else {
					fprintf(stderr, "%c-? ", cardsym[p->cards[p->stacks[i].cards[j]]]);
					fprintf(file, "%c-? ", cardsym[p->cards[p->stacks[i].cards[j]]]);
				}
			}
			fprintf(stderr, "\n");
			// putchar('\n');
			fprintf(file, "\n");
		}
		if (debuglevel <= 1) break;
	}
	fprintf(stderr, "#======================================================\n");
	fprintf(file, "======================================================\n");
}
/* playStateでカード cardnum が来た時のベストのプレイをして */
/* playState, plans いずれも破壊的に代入する */
/* level で, あと何レベル先読みをするかを決める */
/* 0なら先読みなし */


/*
struct _Moves{
int nMoves;
_Move moves[MAXCARD*2];
};
typedef struct _Moves Moves;
*/
void initMoves(MMoves *moves)
{
	moves->nMoves = 0;
}

void addMove(MMoves *moves, int type, int dest, int src)
{
	moves->moves[moves->nMoves].type = type;
	moves->moves[moves->nMoves].src = src;
	moves->moves[moves->nMoves++].dest = dest;
}

int deckNum[14][4];
void initDeckNum()
{
	int i, j, card;
	for (i = 0; i<4; i++) {
		for (card = j = 12; j >= 0; j--) {
			deckNum[card][i] = j;
			card = (card - i - 1 + 13) % 13;
		}
		deckNum[13][i] = 100; /* sentinel */
	}
}

/* どの nodeがどの台に属するかも */
/* カードが置かれている場合は,単純に1カード1 node(ただしサイズが0)とする */
/* るーぷなどがあるばあいは 0をかえす */


void showplanx(Plan *p) {
	cout << "sowplan " << p->maxIndex;
	for (int i = 0; i < 10; i++) cout << " " << p->decks[i];
	cout << endl;
}
void showPlaystatesx(PlayState *p) {
	cout << "sowpplaysrte " << p->nCards;
	for (int i = 0; i < p->nCards; i++) cout << " " << p->cards[i];
	cout << endl;

}
void showNodex(Node *p)
{
	int i;
	cout << "node=" << p->nCards << " in=" << p->inNodes << "  eds=" << p->nEdges << endl;
	;
	cout << "edge=";
	for (i = 0; i < p->nEdges; i++) cout << " " << p->edges[i]; cout << endl;
}
void showStatex(State *p) {
	cout << "State nNodes=" << p->nNodes << endl;
	for (int i = 0; i < p->nNodes; i++) showNodex(&p->nodes[i]);

}

int planToState(State *state, int *nodeToDeck, PlayState *playState, Plan *plan)
{
	int deckCards[NDECKS][13]; /* -1 は使用, 0は未使用,正の値は Node番号 */
	int deck, nnodes, cardNum;
	int i, j, k, card, node;
	Node *lastNode, *pNode0;
	//  showplanx(plan);
	//  showPlaystatesx(playState);
	memset(deckCards, 0, sizeof(deckCards));
	for (i = 0; i<playState->nCards; i++) {
		card = playState->cards[i];
		deck = plan->decks[i];
		if (deck >= 0)
			deckCards[deck][deckNum[card][deck]] = -1;
	}
	state->nodes[0].nCards = 0;
	state->nodes[0].mask = 0;
	state->nodes[0].nEdges = 0;
	for (i = 0, nnodes = 1; i<NDECKS; i++) {
		lastNode = &(state->nodes[0]);
		j = playState->deck[i];
		/* すぐに出せるカードは積む候補に入れない */
		if (j<13 && deckCards[i][j] == 0) {
			j++;
		}
		for (; j<13; j++) {
			if (deckCards[i][j] == 0) {
				for (k = j; k<13 && deckCards[i][k] == 0; k++) {
					deckCards[i][k] = nnodes;
				}
				pNode0 = &(state->nodes[nnodes]);
				pNode0->mask = 0;
				pNode0->nCards = k - j;
				pNode0->nEdges = 0;
				lastNode->edges[lastNode->nEdges++] = nnodes;
				lastNode = pNode0;
				nodeToDeck[nnodes++] = i;
				j = k - 1;
			}
			else {
				deckCards[i][j] = nnodes;
				pNode0 = &(state->nodes[nnodes]);
				pNode0->mask = 0;
				pNode0->nCards = 0;
				pNode0->nEdges = 0;
				lastNode->edges[lastNode->nEdges++] = nnodes;
				lastNode = pNode0;
				nodeToDeck[nnodes++] = i;
			}
		}
	}
	for (i = 0; i<NSTACK; i++) {
		lastNode = NULL;
		for (j = playState->stacks[i].nCards - 1; j >= 0; j--) {
			card = playState->stacks[i].cards[j];
			cardNum = playState->cards[card];
			deck = plan->decks[card];
			node = deckCards[deck][deckNum[cardNum][deck]];
			/* ちょっとここはいいかげん．ようかくにん */
			if (node == -1) {
				return 0;
			}
			if (lastNode != NULL) {
				lastNode->edges[lastNode->nEdges++] = node;
			}
			pNode0 = &(state->nodes[node]);
			pNode0->mask = (1 << i);
			lastNode = pNode0;
		}
	}
	state->nNodes = nnodes;
	/* check loops */
	for (i = 0; i<nnodes; i++) {
		int decendants[MAXNODE];
		memset(decendants, 0, sizeof(decendants));
		pNode0 = &(state->nodes[i]);
		for (j = 0; j<pNode0->nEdges; j++) {
			markDecendants(state, pNode0->edges[j], decendants);
		}
		if (decendants[i] == 1) {
			return 0;
		}
	}
	setMasks(state);
	for (i = 0; i<nnodes; i++) {
		if (state->nodes[i].nCards>0 && state->nodes[i].mask == ALLBITSET) return 0;
	}
	// showStatex(state);

	return 1;
}

/* 次にKが続けて出た時に, 現在の plan でいけるかどうか? */

int canPutK(PlayState *playState, Plan *plan)
{
	int kRest = playState->cardHist[12];
	int nodeToDeck[MAXCARD];
	PlayState tmpPlayState;
	State tmpstate;
	int i, j, nCards, deck;
	int usedK[4];
	int deckK[4];
	int kIndex;
	Stack *stack;

	/* K がすべて出ていたら OK */
	if (kRest == 0) return 1;
	nCards = playState->nCards;
	memset(usedK, 0, sizeof(usedK));
	for (i = 0; i<nCards; i++)
		if (playState->cards[i] == 12)
			usedK[plan->decks[i]] = 1;
	for (kIndex = i = 0; i<NDECKS; i++)
		if (usedK[i] == 0) deckK[kIndex++] = i;
	for (j = 0; j<kRest; j++)
		plan->decks[nCards + j] = deckK[j];
	for (i = NSTACK - 1; i >= 0; i--) {
		tmpPlayState = *playState;
		stack = &(tmpPlayState.stacks[i]);
		for (j = 0; j<kRest; j++) {
			tmpPlayState.cards[nCards + j] = 12;
			pushStack(stack, nCards + j);
		}
		tmpPlayState.nCards = nCards + kRest;
		tmpPlayState.cardHist[12] = 0;
		if (planToState(&tmpstate, nodeToDeck, &tmpPlayState, plan) != 0) return 1;
	}
	return 0;
}


/* それぞれの台ごとの State を計算する */
/* 2種類のintersection の State を計算する */
/* ab * ac * ad * bc * bd * cd / ((a * b * c * d)^2 を返す */

int countEPS0;
int countEPS1;
int countEPS2;
double EvalPlayStatePlan(PlayState *playState, Plan *plan)
{
	State tmpstate, tmpstate1;
	double allprob = 1.0, prob;
	int nodeToDeck[MAXCARD];
	int i, j, k, packedSize;
	unsigned int packed[1024];
	double ratio = 1.0;

	if (debuglevel > 2)
		showPlayStatePlan(playState, plan);
	countEPS0++;
	if (planToState(&tmpstate, nodeToDeck, playState, plan) == 0) return 0.0;
	countEPS1++;
#if 0
	if (!canPutK(playState, plan)) ratio = 10e-5;
#else
	for (k = i = 0; i<tmpstate.nNodes; i++) {
		if (tmpstate.nodes[i].nCards >0)
			k |= tmpstate.nodes[i].mask;
		if (k == ALLBITSET) {
#if 0
			printf("allbit\n");
			showPlayStatePlan(playState, plan);
			showState(&tmpstate);
#endif
			return 0.0;
		}
	}
#endif
	if (debuglevel > 3)
		showState(&tmpstate);
	countEPS2++;
	for (i = 0; i<NDECKS - 1; i++)
		for (j = i + 1; j<NDECKS; j++) {
			copyState(&tmpstate1, &tmpstate);
			for (k = 0; k<tmpstate1.nNodes; k++) {
				if (nodeToDeck[k] != i && nodeToDeck[k] != j) {
					tmpstate1.nodes[k].nCards = 0;
				}
			}
			//  cout<<" bunsi ["<<i<<" "<<j<<"] ";
			normalizeState(&tmpstate1);
			prob = stateProb(&tmpstate1);
			if (prob<10e-10) return 0.0;
			allprob *= prob;
		}
	for (i = 0; i<NDECKS; i++) {
		copyState(&tmpstate1, &tmpstate);
		for (k = 0; k<tmpstate1.nNodes; k++) {
			if (nodeToDeck[k] != i) {
				tmpstate1.nodes[k].nCards = 0;
			}
		}
		//cout << "bumbo " << i << " ";
		normalizeState(&tmpstate1);
		prob = stateProb(&tmpstate1);
		if (prob<10e-10) return 0.0;
		allprob /= prob * prob;

	}
	if (debuglevel >= 3)
		printf("allprob=%e\n", allprob);
	// cin >> i;
	return allprob * ratio;
}

double EvalPlayStateLeaf(PlayState *playState, Plans *plans)
{
	int i, j, index, undefIndex;
	int nCards = playState->nCards;
	double prob, allprob;
	Plan tmpplans[MAXPLAN*NDECKS];
	int perms[MAXPLAN*NDECKS];
	int tmpperm;
	int usedDeck[4], cardNum;
	double tmpprob;
	double maxprob = 0.0;
	int maxIndex = 0;

	if ((undefIndex = plans->undefIndex) < 0) {
		for (allprob = 0.0, index = i = 0; i<plans->nPlans; i++) {
			prob = EvalPlayStatePlan(playState, &(plans->plans[i]));
#ifdef PROB_INDEPENDENT
			allprob += (1 - allprob)*prob;
#else
			if (allprob<prob) allprob = prob;
			plans->plans[i].prob = prob;
#endif
			if (prob>10e-40) {
				if (index != i)
					copyPlan(&(plans->plans[index]), &(plans->plans[i]), nCards);
				if (maxprob<prob) {
					maxprob = prob;
					maxIndex = index;
				}
				index++;
			}
		}
		plans->nPlans = index;
		if (index>0) {
			copyPlan((&tmpplans[0]), &(plans->plans[maxIndex]), nCards);
			copyPlan(&(plans->plans[maxIndex]), &(plans->plans[0]), nCards);
			copyPlan(&(plans->plans[0]), (&tmpplans[0]), nCards);
		}
		addHashPS(playState, allprob);
		return allprob;
	}
	plans->undefIndex = -1;
	cardNum = playState->cards[undefIndex];
	for (allprob = 0.0, index = i = 0; i<plans->nPlans; i++) {
		memset(usedDeck, 0, sizeof(usedDeck));
		for (j = 0; j<nCards; j++) {
			if (playState->cards[j] == cardNum && j != undefIndex) {
				usedDeck[plans->plans[i].decks[j]] = 1;
			}
		}
		for (j = 0; j<NDECKS; j++) {
			if (usedDeck[j]) continue;
			plans->plans[i].decks[undefIndex] = j;
			prob = EvalPlayStatePlan(playState, &(plans->plans[i]));
#ifdef PROB_INDEPENDENT
			allprob += (1 - allprob)*prob;
#else
			if (allprob<prob) allprob = prob;
#if 0
			if (prob> 0.99999999) {
				addHashPS(playState, prob);
				return prob;
			}
#endif
#endif
			if (prob>10e-40) {
				copyPlan((&tmpplans[index]), &(plans->plans[i]), nCards);
				tmpplans[index++].prob = prob;
			}
		}
	}
	for (i = 0; i<index; i++) perms[i] = i;
	for (i = 1; i<index; i++) {
		tmpperm = perms[i];
		tmpprob = tmpplans[tmpperm].prob;
		for (j = i; j >= 1; j--) {
			if (tmpplans[perms[j - 1]].prob >= tmpprob) break;
			perms[j] = perms[j - 1];
		}
		perms[j] = tmpperm;
	}
	if (index>maxplan) index = maxplan;
	for (i = 0; i<index; i++) {
		copyPlan(&(plans->plans[i]), &tmpplans[perms[i]], nCards);
		if (plans->plans[i].maxIndex<i) plans->plans[i].maxIndex = i;
	}
	plans->nPlans = index;
	addHashPS(playState, allprob);
	return allprob;
}

int noplan = 0;
double bestPlay(PlayState *playState, Plans *plans, int level);
double EvalPlayState(PlayState *playState, Plans *plans, int level)
{
	int i;
	double revf, prob, allprob;
	PlayState tmpstate;
	Plans tmpplans;
	HashPS *ptr;

	if (plans->nPlans == 0) { /* 割り当てのプランが残っていない */
		return 0.0;
	}
	/* can put K */
	for (i = 0; i<NSTACK; i++)
		if (playState->stacks[i].nCards == 0 ||
			playState->cards[playState->stacks[i].cards[0]] == 12) break;
	if (i == NSTACK) {
		return 0.0;
	}
	if ((ptr = findHashPS(playState)) != NULL) {
		return ptr->prob;
	}
	if (level == 0 || playState->nCards == MAXCARD) {
		return EvalPlayStateLeaf(playState, plans);
	}
	if (!noplan && plans->undefIndex >= 0)
		EvalPlayStateLeaf(playState, plans);
	revf = 1.0 / (double)(MAXCARD - playState->nCards);
	for (allprob = 0.0, i = 0; i<13; i++) {
		if (playState->cardHist[i]) {
			tmpstate = *playState;
			tmpstate.cards[tmpstate.nCards++] = i;
			tmpstate.cardHist[i]--;
			copyPlans(&tmpplans, plans, tmpstate.nCards);
			prob = bestPlay(&tmpstate, &tmpplans, level - 1);
			allprob += prob * revf*(double)playState->cardHist[i];
		}
	}
	if (debuglevel>20) {
		showPlayStatePlans(playState, plans);
	}
	addHashPS(playState, allprob);
	return allprob;
}

void uniqPlans(Plans *plans, int nCards)
{
	int hvals[MAXPLAN*NDECKS];
	int *decks, *decks1;
	int i, j, nPlans, index, k, hval;

	nPlans = plans->nPlans;
	for (i = index = 0; i<nPlans; i++) {
		decks = &(plans->plans[i].decks[0]);
		for (hval = 0, j = 0; j<nCards; j++) {
			hval = (hval << 2) + (hval >> 2);
			hval += decks[j];
		}
		for (j = 0; j<index; j++) {
			if (hvals[j] == hval) {
				decks1 = &(plans->plans[j].decks[0]);
				for (k = 0; k<nCards; k++)
					if (decks1[k] != decks[k]) break;
				if (k == nCards) goto deleted;
			}
		}
		if (i != index)
			plans->plans[index] = plans->plans[i];
		if (plans->plans[index].maxIndex<index) plans->plans[index].maxIndex = index;
		hvals[index++] = hval;
	deleted:;
	}
	plans->nPlans = index;
}

void prunePlans(PlayState *playState, Plans *plans, int deck, int card)
{
	int index, i, j;
	int cardNum = playState->cards[card];

	for (index = i = 0; i<plans->nPlans; i++) {
		for (j = 0; j<playState->nCards; j++) {
			if (j != card &&
				plans->plans[i].decks[j] == deck &&
				playState->cards[j] == cardNum)
				goto bad;
		}
		if (i != index) {
			copyPlan(&(plans->plans[index]), &(plans->plans[i]), playState->nCards);
		}
		if (plans->plans[index].maxIndex<index) plans->plans[index].maxIndex = index;
		plans->plans[index].decks[card] = deck;
		index++;
	bad:;
	}
	plans->nPlans = index;
	if (card != playState->nCards - 1)
		uniqPlans(plans, card);
}


double checkMoves(PlayState *playState, Plans *plans, MMoves *moves, int level)
{
	int i, j, k;
	int top, topcard, card, deck;
	PlayState tmpstate;
	Plans tmpplans, maxplans;
	MMoves tmpmoves, maxmoves;
	double prob, maxprob;
	int undefIndex = plans->undefIndex;
	//printf("checkmove %d\n",level);
	maxprob = 0.0;
	if (plans->nPlans == 0) return 0.0;
	for (i = 0; i<NSTACK; i++) {
		if ((top = topOfStack(&(playState->stacks[i]))) == NOCARD) continue;
		topcard = playState->cards[top];
		deck = plans->plans[0].decks[top];
		if (top != undefIndex &&
			deckNum[topcard][deck] == playState->deck[deck]) {
			tmpmoves = *moves;
			tmpstate = *playState;
			copyPlans(&tmpplans, plans, playState->nCards);
			addMove(&tmpmoves, XMOVE, deck, i);
			card = popStack(&(tmpstate.stacks[i]));
			if (card == undefIndex) {
				tmpplans.undefIndex = -1;
				for (j = 0; j<tmpplans.nPlans; j++)
					tmpplans.plans[j].decks[card] = deck;
			}
			else {
				prunePlans(&tmpstate, &tmpplans, deck, card);
			}
			tmpstate.deck[deck]++;
			prob = checkMoves(&tmpstate, &tmpplans, &tmpmoves, level);
			if (prob>maxprob) {
				maxprob = prob;
				maxmoves = tmpmoves;
				copyPlans(&maxplans, &tmpplans, playState->nCards);
				if (maxprob>0.99999999) goto ret;
			}
		}
	}
	for (i = 0; i<NSTACK; i++) {
		if ((top = topOfStack(&(playState->stacks[i]))) == NOCARD) continue;
		//printf("checkmove i=%d\n",i);
		topcard = playState->cards[top];
		deck = plans->plans[0].decks[top];
		for (j = 0; j<NDECKS; j++) {
			if (j == deck && top != undefIndex) continue;
			//  printf("checkmove j=%d\n",j);
			if (deckNum[topcard][j] == playState->deck[j]) {
				tmpmoves = *moves;
				tmpstate = *playState;
				copyPlans(&tmpplans, plans, playState->nCards);
				addMove(&tmpmoves, XMOVE, j, i);
				card = popStack(&(tmpstate.stacks[i]));
				if (card == undefIndex) {
					tmpplans.undefIndex = -1;
					for (k = 0; k<tmpplans.nPlans; k++)
						tmpplans.plans[k].decks[card] = j;
				}
				else {
					prunePlans(&tmpstate, &tmpplans, j, card);
				}
				tmpstate.deck[j]++;
				//	printf("====card=%d \n",card);// donot reach here
				prob = checkMoves(&tmpstate, &tmpplans, &tmpmoves, level);
				if (prob>maxprob) {
					maxprob = prob;
					maxmoves = tmpmoves;
					copyPlans(&maxplans, &tmpplans, playState->nCards);
					if (maxprob>0.99999999) goto ret;
				}
			}
		}
	}
	prob = EvalPlayState(playState, plans, level);
	if (maxprob<10e-40 || prob>maxprob) {
		return prob;
	}
ret:
	*moves = maxmoves;
	copyPlans(plans, &maxplans, playState->nCards);
	return maxprob;
}

#if 0
void newPutPlans(PlayState *playState, Plans *plans, int stack, int card)
{
	Plan tmpplans[MAXPLAN*NDECKS];
	int perms[MAXPLAN*NDECKS];
	int i, j, k, index, tmpperm;
	double prob, tmpprob;
	int cardNum = playState->cards[card];

	for (index = i = 0; i<plans->nPlans; i++) {
		for (j = 0; j<NDECKS; j++) {
			for (k = 0; k<card; k++) {
				if (playState->cards[k] == cardNum && plans->plans[i].decks[k] == j)
					goto noplan;
			}
			if (cardNum == 12) {
				Stack *stackP = &(playState->stacks[stack]);
				if (stackP->nCards >= 2 && playState->cards[stackP->cards[stackP->nCards - 2]] == 12 && j>plans->plans[i].decks[stackP->cards[stackP->nCards - 2]]) {
					continue;
				}
			}
			copyPlan(&tmpplans[index], &(plans->plans[i]), card);
			tmpplans[index].decks[card] = j;
			prob = EvalPlayStatePlan(playState, &tmpplans[index]);
			if (prob >= 10e-40) {
				tmpplans[index++].prob = prob;
			}
		noplan:;
		}
	}
	for (i = 0; i<index; i++) perms[i] = i;
	for (i = 1; i<index; i++) {
		tmpperm = perms[i];
		tmpprob = tmpplans[tmpperm].prob;
		for (j = i; j >= 1; j--) {
			if (tmpplans[perms[j - 1]].prob >= tmpprob) break;
			perms[j] = perms[j - 1];
		}
		perms[j] = tmpperm;
	}
	if (index>maxplan) index = maxplan;
	for (i = 0; i<index; i++) {
		copyPlan(&(plans->plans[i]), &tmpplans[perms[i]], card);
		if (plans->plans[i].maxIndex<i) plans->plans[i].maxIndex = i;
	}
	plans->nPlans = index;
}
#endif
int YSW = 0;
MMoves maxmovescopy;
int MoveCardsList[52];
int cards[52];
double bestPlay(PlayState *playState, Plans *plans, int level)
{
	double maxprob = 0, prob;
	PlayState tmpstate;
	Plans tmpplans, maxplans;
	int card = playState->nCards - 1, i, j, deck;
	int cardnum = playState->cards[card];
	int count;
	MMoves tmpmoves, maxmoves;

	/* 台に出すことができるか? */
	for (i = 0; i<NDECKS; i++) {
		if (playState->deck[i] == deckNum[cardnum][i]) { /* まさに出せる */
			initMoves(&tmpmoves);
			tmpstate = *playState;
			tmpstate.deck[i]++;
			copyPlans(&tmpplans, plans, card);
			/* plans の中から i に出すつもりだったPlan を外す */
			prunePlans(&tmpstate, &tmpplans, i, card);
			/* plan に今の情報を加える */
			for (j = 0; j<tmpplans.nPlans; j++) {
				tmpplans.plans[j].decks[card] = i;
			}
			addMove(&tmpmoves, DECK, i, 0);
			prob = checkMoves(&tmpstate, &tmpplans, &tmpmoves, level);
			if (prob>maxprob) {
				copyPlans(&maxplans, &tmpplans, card + 1);
				maxprob = prob;
				maxmoves = tmpmoves;
			}
		}
	}
	/* スタックに置く */
	for (i = 0; i<NSTACK; i++) {
		initMoves(&tmpmoves);
		copyPlans(&tmpplans, plans, card);
		tmpstate = *playState;
		pushStack(&(tmpstate.stacks[i]), card);
#if 0
		newPutPlans(&tmpstate, &tmpplans, i, card);
#endif
		if (noplan && tmpplans.undefIndex >= 0) break;
		tmpplans.undefIndex = card;
		addMove(&tmpmoves, STACK, i, 0);
		prob = checkMoves(&tmpstate, &tmpplans, &tmpmoves, level);
		if (prob>maxprob) {
			copyPlans(&maxplans, &tmpplans, card + 1);
			maxprob = prob;
			maxmoves = tmpmoves;
		}
	}
	if (maxprob<10e-40) return 0.0;
	//  printf("n=%d\n",maxmoves.nMoves);
	copyPlans(plans, &maxplans, card + 1);
	//move 表示のためにコピーを入れた  2013.8.15 ここでは表示しない
	memcpy((char *)&maxmovescopy, (char *)&maxmoves, sizeof(MMoves));
	count = 0;
	for (i = 0; i<maxmoves.nMoves; i++) {
		switch (maxmoves.moves[i].type) {
		case DECK:
			deck = maxmoves.moves[i].dest;
			playState->deck[deck]++;
			if (YSW)   printf(" deck:%d\n", deck + 1);
			break;
		case STACK:
			if (YSW)	 printf(" stack:%d\n", maxmoves.moves[i].dest + 1);
			pushStack(&(playState->stacks[maxmoves.moves[i].dest]), card);
			break;
		case XMOVE:
			card = popStack(&(playState->stacks[maxmoves.moves[i].src]));
			MoveCardsList[count++] = card;
			deck = maxmoves.moves[i].dest;
			playState->deck[deck]++;
			if (YSW) printf(" move:%d==>%d\n", maxmoves.moves[i].src + 1, deck + 1);
			break;
		}
	}


	return maxprob;
}

/* Stateのtransposition table を作る */
double bestPlayTop(PlayState *playState, Plans *plans, int level)
{
	double prob;
	if (hashPStable == NULL)
		initHashPS();

	prob = bestPlay(playState, plans, level);
	//--------------------------------------------------
	int i, deck, card, count = 0;
	char pipebuf[200];
	char *pbuf = pipebuf;
	int bcount = 1;
	for (i = 0; i<maxmovescopy.nMoves; i++) {
		switch (maxmovescopy.moves[i].type) {
		case DECK:
			deck = maxmovescopy.moves[i].dest;
			//   playState->deck[deck]++;
			fprintf(file, " deck:%d ", deck + 1);
			if (pipe) *pbuf++ = '1' + deck;
			break;
		case STACK:
			if (pipe) *pbuf++ = 'W' + maxmovescopy.moves[i].dest+4-NSTACK;
			fprintf(file, " stack:%d ", maxmovescopy.moves[i].dest + 1);
			// pushStack(&(playState->stacks[maxmovescopy.moves[i].dest]),card);
			break;
		case XMOVE:
			card = MoveCardsList[count++];
			deck = maxmovescopy.moves[i].dest;
			if (pipe) {
				*pbuf++ = '/';
				*pbuf++ = 'W' + maxmovescopy.moves[i].src+4 - NSTACK;
				*pbuf++ = '1' + maxmovescopy.moves[i].dest;
			}
			if (bcount == 5) { bcount = 0; fprintf(file, "\n"); }
			bcount++;

			//    playState->deck[deck]++;
			//printf("bestplay move(%c):%d==>%d\n", cardsym[cards[card]], maxmovescopy.moves[i].src,deck);
			fprintf(file, " move(%c):%d==>%d", cardsym[cards[card]],
				maxmovescopy.moves[i].src + 1, deck + 1);
			break;
		}
	}
	fprintf(file, "\n");
	if (pipe) {
		//pipe option　-x で　answerを返す　2014.7.3
		*pbuf++ = 0;


		puts(pipebuf);// '\n' は　putsがつける
					  // showPlayStatePlans(playState, plans);
	}
	showPlayStatePlans(playState, plans);

	//--------------------------------------------------



	//-----------------------------------------------------
	int n;
	n = plans->nPlans;
	Plan *p = plans->plans + n - 1;
	//printf("nm=%d\n",p->nMoves);
	clearHashPS();
	return prob;
}
int chartable(char c) {
	int j;
	for (j = 0; j < 13; j++)if ("A23456789TJQK"[j] == c)break;
	if (j == 13) return -1;
	return j;
}
bool checkCharacter(int ca[]) {
	int i, j, count[13];
	for (i = 0; i < 13; i++) count[i] = 0;
	for (i = 0; i < 52; i++) {

		if (ca[i]<0 || ca[i]>12)return false;
		count[ca[i]]++;
	}
	for (i = 0; i < 13; i++) if (count[i] != 4)
	{
		fprintf(stderr, "i=%d count=%d\n", i, count[i]);
		return false;
	}
	return true;
}

/* cards 配列を乱数で初期化する */
void initSeq(int ss)
{
	int i, j, k, tmp;
	char c;


	for (i = 0; i < 52; i++) {
		c = inputdata[ss][i];
		for (j = 0; j < 13; j++)if ("A23456789TJQK"[j] == c)break;
		if (j == 13) break;
		cards[i] = j;

	}	if (i < 52) cards[i] = 0;


	/*
	for(i=0;i<13;i++)
	for(j=0;j<4;j++){
	cards[j*13+i]=i;
	}
	for(k=0;k<10;k++){
	for(i=0;i<51;i++){
	j=i+(rand()%(52-i));
	tmp=cards[j];
	cards[j]=cards[i];
	cards[i]=tmp;
	}
	}
	*/
	fprintf(file, "problem %d\n", ss);
	for (i = 0; i<52 && cards[i]>0; i++) fprintf(file, "%c", cardsym[cards[i]]); fprintf(file, "\n");

}


/* 成功した時は1を失敗した時は0を返す */
int computerPlay(int level, int ss, int inputmode)
{
	PlayState topState;
	Plans topPlans;
	int i;
	double prob;
	if (inputmode == 1) {
		fprintf(stderr, "ss=%d txt=%s\n", ss, px[ss]);
		for (i = 0; i < 52; i++) {
			cards[i] = 13;
			if (px[ss][i] == 0) break;
			cards[i] = chartable(px[ss][i]);
			//	  fprintf(stderr, "%d ", cards[i]);
		}
		fprintf(file, "problem %d\n", ss);
		for (i = 0; i < 52; i++)
		{
			if (cards[i]<13)
				fprintf(file, "%c", cardsym[cards[i]]); fprintf(file, "\n");
		}
	}
	else if (pipe == 0 && ss >= 0) {
		initSeq(ss);// array acrds[] に問題」をセット
		if (!checkCharacter(cards)) { fprintf(stderr, "invalid character \n"); return 0; }
	}
	else if (pipe) fprintf(stderr, "pipe option\n");
	fprintf(stderr, "1option\n");
	initPlayState(&topState);
	topPlans.nPlans = 1;
	topPlans.undefIndex = -1;
	for (i = 0; i<52; i++) {
		noplan = 0;
		if (pipe == 0) {
			if (cards[i] == 13) {
				fprintf(stderr, "short problem\n"); exit(1);
			}
			fprintf(file, "%d : %c", i + 1, cardsym[cards[i]]);
			// fprintf(stderr, "%d : %c\n", i + 1, cardsym[cards[i]]);
			if (debuglevel > 0)     fprintf(stderr, "#%d : %c\n", i + 1, cardsym[cards[i]]);
		}
		else {
			char work[3];
			fprintf(stderr, "#enter card step=%d ", i + 1);//pipe option 2014.7.5
														   // gets(work);
			std::cin >> work;
			int j;
			char c = work[0];
			c = toupper(c);
			for (j = 0; j < 13; j++)if ("A23456789TJQK"[j] == c)break;
			cards[i] = j;
			if (j >= 13)fprintf(stderr, "#invalid card j=%d", j);
		}
		topState.cards[topState.nCards++] = cards[i];
		topState.cardHist[cards[i]]--;
		prob = bestPlayTop(&topState, &topPlans, level);

		if (prob<10e-40) {
			noplan = 1;
			prob = bestPlayTop(&topState, &topPlans, level + 1);

			if (prob<10e-40)
				return 0;
		}
		if (debuglevel > -2) {
			fprintf(stderr, "\n");
#if 1
			if (debuglevel>-21) {
				fprintf(stderr, "#prob=%e, plans=%d maxIndex=%d\n", prob, topPlans.nPlans, topPlans.plans[0].maxIndex);
				fprintf(stderr, "EvalPlayStatePlan(%d/%d/%d)\n", countEPS0, countEPS1, countEPS2);
				//hashsize
				int jj;
				//	if (i == 14) cin >> jj;//cin >> i;
			}
#else
			fprintf(stderr, "#prob=%e, plans=%d\n", prob, topPlans.nPlans);
#endif

		}

	}
	return 1;
}
int cardtoint(char c) {
	int n = 0;
	const char *cc = "A23456789TJQK";
	for (n = 0; n<13; n++) if (c == cc[n]) break;
	return n;
}
int main(int ac, char **ag)
{
	int i, resultCount, count = 100, start = 0, result;
	int searchLevel = 0; /* 0手先まで読む */
	char fn[80];

	int src = 0;
	initDatFile((NSTACK==3)?DATFILE3:DATFILE4);
	initDeckNum();
	fprintf(stderr, "Version 7.10 2018/01/14\n");
	file = fopen("trace.txt", "w");
	if (!file) { printf("ERR"); exit(1); }
	//  cout << "version Relese 6,3 step=1" << endl;


	FILE *ff;
	setbuf(stdout, NULL);
	char problemtxt[53];
	for (i = 1; i<ac; i++) {
		if (ag[i][0] == '-') {
			switch (ag[i][1]) {
			case 'p':
				strcpy(px[0], ag[i + 1]);
				count = 1; start = 0;
				maxplan = 200;

				searchLevel = 1;
				debuglevel = 1;
				src = 1;
				break;
			case 'x'://pipe
				pipe = 1;
				start = -1;
				count = 1;
				src = 2;
				maxplan = 200;
				searchLevel = 1;
				debuglevel = 0;
				break;
			case 'c':
				if (isdigit(ag[i][2])) count = atoi(&ag[i][2]);
				else if (i + 1<ac) count = atoi(&ag[++i][0]);
				break;
			case 'd':
				if (isdigit(ag[i][2])) debuglevel = atoi(&ag[i][2]);
				else if (i + 1<ac) debuglevel = atoi(&ag[++i][0]);
				break;
			case 'l':
				if (isdigit(ag[i][2])) searchLevel = atoi(&ag[i][2]);
				else if (i + 1<ac) searchLevel = atoi(&ag[++i][0]);
				break;
			case 'm':
				if (isdigit(ag[i][2])) maxplan = atoi(&ag[i][2]);
				else if (i + 1<ac) maxplan = atoi(&ag[++i][0]);
				if (maxplan>MAXPLAN) {
					printf("maxplan is rounded to %d\n", MAXPLAN);
					maxplan = MAXPLAN;
				}
				break;
			case 's':
				if (isdigit(ag[i][2])) start = atoi(&ag[i][2]);
				else if (i + 1<ac) start = atoi(&ag[++i][0]);
				break;
			case 'f':
				ff = fopen(ag[++i], "r");
				if (!ff) {
					strcpy(fn, ag[i]); strcat(fn, ".txt");
					ff = fopen(fn, "r");
					if (!ff) { printf("error file"); exit(1); }
				}
				for (count = 0; count < 100; count++) {
					if (!fgets(px[count], 603, ff)) break;
					//	  fprintf(stderr, "count=%d %s\n", count,px[count]);//OK
				}
				src = 1;
				//  fgets(problemtxt,53,ff);
				fclose(ff);
				//  printf("\nfile=%s\n", problemtxt);
				fprintf(stderr, " count=%d\n", count);
				start = 0;

				maxplan = 200;
				searchLevel = 1;
				debuglevel = 1;
				break;
			}

		}
		else if (isdigit(ag[i][0])) {
			count = atoi(ag[i]);
		}
	}
	if (ac == 1) {
		maxplan = 50;
		count = 1;
		start = 0;
		searchLevel = 1;
		debuglevel = 1;//	  1;
	}
	if (start<0) {
		for (i = 0; i<52; i++) cards[i] = cardtoint(px[0][i]);
		for (i = 0; i<52; i++) fprintf(file, "%c", cardsym[cards[i]]); fprintf(file, "\n");

	}
	long t1, t2;


	if (pipe) setbuf(stdout, NULL);



	//for(i=0;i<start;i++) rand();
	for (resultCount = i = 0; i<count; i++, start++) {
		// 	srandom(start+i);
		t1 = GetTickCount();

		result = computerPlay(searchLevel, start, src);
		resultCount += result;
		t2 = GetTickCount();
		fprintf(stderr, "#%d %c time=%d sec\n", start + i, "XO"[result], (int)(t2 - t1) / 1000);
		fprintf(file, "#%d %c time=%d sec\n", start + i, "XO"[result], (int)(t2 - t1) / 1000);
		fflush(stdout);
	}
	fprintf(stderr, "#%d/%d\n", resultCount, count);
	fprintf(file, "Result %d/%d\n", resultCount, count);
	fclose(file);
	return 0;
}
/*

*/




