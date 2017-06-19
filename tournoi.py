from os import popen
from threading import Thread
import time
def main():
	global somme, somme2, total, win, win2
	popen("make")
	somme = 0
	total = 0
	somme2 = 0
	
	win = 0
	win2 = 0
	
	N = 200

	class truc(Thread):
		def run(self):
			global somme, somme2, total, win, win2
			f = popen("stechec2-run config4.yml")
			score_old = -1000;
			while True:
				line = f.readline()
				if "player: Ancien" in line:
					line2 = f.readline()
					score = int(line2[7:])
					somme2 += score
					score_old = score
					print(score)
				if "player: Nouveau" in line:
					line2 = f.readline()
					score = int(line2[7:])
					somme += score
					if(score_old > score):
						win2 += 1
					elif(score_old < score):
						win += 1
					elif(score_old == score):
						win2 += 0.1
						win += 0.1
					total += 1
					print("------- (old vs new)")
					print("old: ", somme2/total)
					print("new: ", somme/total)
					print("diff: ",somme/total - somme2/total)
					print("old: ", win2)
					print("new: ", win)
					break
			time.sleep(1)
			f.close()
	class truc2(Thread):
		def run(self):
			global somme, somme2, total, win, win2
			f = popen("stechec2-run config5.yml")
			score_nv = -1000;
			while True:
				line = f.readline()
				if "player: Nouveau" in line:
					line2 = f.readline()
					score = int(line2[7:])
					somme += score
					score_nv = score
				if "player: Ancien" in line:
					line2 = f.readline()
					score = int(line2[7:])
					somme2 += score
					if(score_nv < score):
						win2 += 1
					elif(score_nv > score):
						win += 1
					elif(score_nv == score):
						win2 += 0.1
						win += 0.1
					total += 1
					print("------- (new vs old)")
					print("old: ", somme2/total)
					print("new: ", somme/total)
					print("diff: ",somme/total - somme2/total)
					print("old: ", win2)
					print("new: ", win)
					break
			time.sleep(1)
			f.close()
	for _ in range(N):
		a = truc()
		a.start()
		time.sleep(0.1)
		a = truc2()
		a.start()
		time.sleep(0.1)
		a = truc()
		a.start()
		time.sleep(0.1)
		a = truc2()
		a.start()
		time.sleep(3)

	while total < N:
		time.sleep(0.5)
main()
