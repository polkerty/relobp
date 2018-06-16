#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <map>



using namespace std;



const double DEFAULT_CAP = 1.0, DEFAULT_OPP = 1.0;
const int PLAYER_PLAY_RADIUS = 30, PITCHER_PLAY_RADIUS = 50;
const int ITER = 20;


// 0 == OBP
// 1 == SLG
// 2 == AVG
const int STAT_TYPE = 0;

const bool OUTPUT_PLAY_STRENGTH = true;

const int MIN_PLAYER_PLAYS = 400, NUM_PLAYERS = 30;

vector<string> captions = {
	"Best hitters for on base (min " + to_string(MIN_PLAYER_PLAYS) + " plays): " ,
	"Best hitters for slugging (min " + to_string(MIN_PLAYER_PLAYS) + " plays): ", 
	"Best hitters for average (min " + to_string(MIN_PLAYER_PLAYS) + " plays): " 
};


struct Play {
	int play_id,
		player,
		pitcher,
		game,
		atbat,
		babip_val,
		iso_val;
	
	//double est_ba;
		
	string event, 
			player_name;
			
	Play ( int pid, string pn, int bat, int pit, string ev, int gm, /* double est, */ int bav, int isv, int abn ) :
		play_id(pid), player(bat), pitcher(pit), event(ev), game(gm), /* est_ba(est), */ babip_val(bav), iso_val(isv), atbat(abn), player_name(pn) {
		
	}
			
		
};


// tools to evaluate plays in the general case
inline int count ( Play play ) {
	switch ( STAT_TYPE ) {
		case 0: // OBP
			return play.babip_val || play.iso_val || play.event == "walk" || play.event == "intent_walk" || play.event == "hit_by_pitch";
		case 1: // SLG
			return play.event == "home_run" ? 4 : play.babip_val + play.iso_val;
		case 2: // AVG
			return play.babip_val || play.iso_val;
		default:
			return 0;
	
	}
}

inline bool valid ( Play play ) {
	switch( STAT_TYPE ) {
		case 0: // OBP
			return !(
				//play.event=="sac_bunt" || 
				play.event=="caught_stealing_2b" || 
				play.event=="caught_stealing_3b" || 
				play.event=="pickoff_caught_stealing_2b" || 
				play.event=="pickoff_caught_stealing_3b" || 
				play.event=="pickoff_1b" || 
				play.event=="pickoff_2b" || 
				play.event=="pickoff_3b"
				
			);
		case 1: // slg
			return !(
				play.event=="caught_stealing_2b" || 
				play.event=="caught_stealing_3b" || 
				play.event=="pickoff_caught_stealing_2b" || 
				play.event=="pickoff_caught_stealing_3b" || 
				play.event=="pickoff_1b" || 
				play.event=="pickoff_2b" || 
				play.event=="pickoff_3b" ||
				play.event=="sac_fly" || 
				play.event=="sac_bunt" || 
				play.event == "walk" || 
				play.event == "intent_walk" || 
				play.event == "hit_by_pitch"
			);
		case 2: // avg
			return !(
				play.event=="caught_stealing_2b" || 
				play.event=="caught_stealing_3b" || 
				play.event=="pickoff_caught_stealing_2b" || 
				play.event=="pickoff_caught_stealing_3b" || 
				play.event=="pickoff_1b" || 
				play.event=="pickoff_2b" || 
				play.event=="pickoff_3b" ||
				play.event=="sac_fly" || 
				play.event=="sac_bunt" || 
				play.event == "walk" || 
				play.event == "intent_walk" || 
				play.event == "hit_by_pitch"
			);
		default:
			break;
	}
}



vector<Play> plays;

void read_csv() {
	
	// "","player_name","player","pitcher","events","game_pk","estimated_ba_using_speedangle","babip_value","iso_value","at_bat_number"
	string header;
	getline(cin, header);
	
	int n = 0;
	
	while ( cin ) {
		
		string line;
		getline(cin, line);
		
		if ( !line.size() ) continue;
		
		vector<string> items;
		
		string chunk = "";
		
		for ( auto ch : line ) {
			if ( ch == ',' ) {
				if ( chunk.size() ) {
					items.push_back(chunk);
					chunk = "";
				}	
			} else if ( ch == '"' ) {
			
			} else {
				chunk += ch;
			}
			
			
		}
		
		items.push_back(chunk);
		
		Play p (
				stoi(items[0]),
				(items[1]),
				stoi(items[2]),
				stoi(items[3]),
				(items[4]),
				stoi(items[5]),
				/*stod(items[6]),*/
				stoi(items[7]),
				stoi(items[8]),
				stoi(items[9])	
		);
		if ( valid (p) ) {
			plays.push_back(	
				p
			);
	  	}
		
	}
	



}



void sort_plays() {
	sort(plays.begin(), plays.end(), [](const Play& a, const Play&b)->bool{
		if ( a.game == b.game ) return a.atbat < b.atbat;
		return a.game < b.game;
	});
}


struct Player {
	string name;
	int id;
	
	
	double avg_cap; 
	double avg_opp;
	
	vector<double> true_measurement;
	vector<double> cap;
	vector<int> plays;
	vector<int> pitcher_opp_id;
	Player () {}
	Player( string n, int i) : name(n), id(i){}
};

struct Pitcher {
	int id;
	
	double avg_cap;
	double avg_opp;
	vector<double> opp;
	vector<int> plays;
	vector<int> player_cap_id;
	Pitcher() {}
	Pitcher( int i  ) : id (i) {}
};


map<int, Player> players;
map<int, Pitcher> pitchers;


void setup_players () {

	for ( int i = 0; i < plays.size(); ++i ) {
		auto p = plays[i];
		if ( players.find(p.player) == players.end() ) {
			players[p.player] = Player (p.player_name, p.player );
		}
		
		players[p.player].cap.push_back(DEFAULT_CAP);
		players[p.player].true_measurement.push_back(0);
		players[p.player].plays.push_back( i );
		players[p.player].pitcher_opp_id.push_back( pitchers[p.pitcher].opp.size() );
		
		if ( pitchers.find(p.pitcher) == pitchers.end() ) {
			pitchers[p.pitcher] = Pitcher (p.pitcher);
		}

		pitchers[p.pitcher].opp.push_back(DEFAULT_OPP);
		pitchers[p.pitcher].plays.push_back( i );		
		pitchers[p.pitcher].player_cap_id.push_back( players[p.player].cap.size() - 1 );
	}
}


// __ just looking at batting average right now. __

void iterate_players () {
		
		//int c = 0;
		
		// iterate over each player individually
		for ( auto iter = players.begin(); iter != players.end(); ++iter/*, ++c*/ ) {
			
			auto& player = iter->second;
			
			// what we're measuring
			double total_opp = 0;
			int total_hits = 0;
			
			int play_cnt = 0;
						
			for ( int i = 0 ; i <= min( (int)player.plays.size() - 1, PLAYER_PLAY_RADIUS  ); ++i ) {
				auto play = plays[player.plays[i]];
				total_opp += pitchers[play.pitcher].opp[ player.pitcher_opp_id[i] ];
				total_hits += count(play);
				
				++play_cnt;
			}

			

			// cap * total_opp == results
			// reality / (opp * pa) = cap
			
			for ( int i = 0; i < player.plays.size(); ++i ) {
				
				auto play = plays[player.plays[i]];
				player.cap[i] = total_hits / (total_opp + 0.0001);
				player.true_measurement[i] = total_hits * 1.0 / play_cnt;
				//if ( c == 1 ) cout << player.cap[i] << "/" << pitchers[play.pitcher].opp[ player.pitcher_opp_id[i] ] << ' ';
				
				// adjust based on radius
				// add?
				
				if ( i + 1 + PLAYER_PLAY_RADIUS < player.plays.size() ) {
					auto play = plays[player.plays[i + 1 + PLAYER_PLAY_RADIUS]];
					total_opp += pitchers[play.pitcher].opp[ player.pitcher_opp_id[i + 1 + PLAYER_PLAY_RADIUS] ];
					total_hits += count(play);
					
					++play_cnt;
				}
				
				if ( i + 1 - PLAYER_PLAY_RADIUS > -1 ) {
					auto play = plays[player.plays[i + 1 - PLAYER_PLAY_RADIUS]];
					total_opp -= pitchers[play.pitcher].opp[ player.pitcher_opp_id[i + 1 - PLAYER_PLAY_RADIUS] ];
					total_hits -= count(play);
					
					--play_cnt;
				}
				
				
				
				
			}
			
			
		}
		//cout << endl << endl;
}


void iterate_pitchers () {
		// iterate over each pitcher individually
		for ( auto iter = pitchers.begin(); iter != pitchers.end(); ++iter ) {
			
			auto& pitcher = iter->second;
			
			// what we're measuring
			double total_cap = 0;
			int total_hits = 0;
						
			for ( int i = 0 ; i <= min( (int)pitcher.plays.size() - 1, PITCHER_PLAY_RADIUS  ); ++i ) {
				auto play = plays[pitcher.plays[i]];
				total_cap += players[play.player].cap[ pitcher.player_cap_id[i] ];
				total_hits += count(play);
			}
			
			
			// opp * total_cap == results
			// reality / (total_cap) = opp
			
			for ( int i = 0; i < pitcher.plays.size(); ++i ) {
				
				pitcher.opp[i] = total_hits / (total_cap + 0.0001);
				
				
				// adjust based on radius
				// add?
				
				if ( i + 1 + PITCHER_PLAY_RADIUS < pitcher.plays.size() ) {
					auto play = plays[pitcher.plays[i + 1 + PITCHER_PLAY_RADIUS]];
					total_cap += players[play.player].cap[ pitcher.player_cap_id[i + 1 + PITCHER_PLAY_RADIUS] ];
					total_hits += count(play);
				}
				
				if ( i + 1 - PITCHER_PLAY_RADIUS > -1 ) {
					auto play = plays[pitcher.plays[i + 1 - PITCHER_PLAY_RADIUS]];
					total_cap -= players[play.player].cap[ pitcher.player_cap_id[i + 1 - PITCHER_PLAY_RADIUS] ];
					total_hits -= count(play);
				}
				
				
			}
			
			
			
		}
	
}


void calculate_stats() {
	for ( auto iter = pitchers.begin(); iter != pitchers.end(); ++iter ) {
			
		auto& pitcher = iter->second;
		double tot_opp = 0;
		for ( int i = 0; i < pitcher.opp.size(); ++i ) tot_opp += pitcher.opp[i];
		pitcher.avg_opp = tot_opp / pitcher.opp.size();
			
	}

	for ( auto iter = players.begin(); iter != players.end(); ++iter ) {
			
		auto& player = iter->second;
		double tot_cap = 0, tot_opp  = 0;
		int tot_weight = 0;
		for ( int i = 0; i < player.cap.size(); ++i ) {
		
			int mul = 1;
			mul += min(PLAYER_PLAY_RADIUS, (int)player.cap.size() - i);
			mul += min(PLAYER_PLAY_RADIUS, i);
			tot_weight += mul;
			tot_cap += mul*player.cap[i];
			tot_opp += mul*( pitchers[plays[player.plays[i]].pitcher].opp[ player.pitcher_opp_id[i] ] );
		}
		player.avg_cap = tot_cap / tot_weight;
		player.avg_opp = tot_opp / tot_weight;
		//cout << player.avg_cap << ' ';
			
	}

}

void show_leaders() {
	vector<Player> pl;
	vector<Pitcher> pi;
	for ( auto iter = pitchers.begin(); iter != pitchers.end(); ++iter ) pi.push_back(iter->second);
	for ( auto iter = players.begin(); iter != players.end(); ++iter ) pl.push_back(iter->second);
	
	
	sort( pl.begin(), pl.end(), [](const Player& a, const Player& b)->bool{ 
		if ( a.plays.size() < MIN_PLAYER_PLAYS && b.plays.size() >= MIN_PLAYER_PLAYS ) {
			return false;
		} else if ( a.plays.size() >= MIN_PLAYER_PLAYS && b.plays.size() < MIN_PLAYER_PLAYS ) {
			return true;
		}
		return a.avg_cap > b.avg_cap;
	});

// 	sort( pi.begin(), pi.end(), [](const Player& a, const Player& b)->bool{ 
// 		return a.avg_opp < b.avg_opp;
// 	});
	
	
	cout << captions[STAT_TYPE] << endl;
	for ( int i = 0; i < NUM_PLAYERS; ++i ) {
		cout << (i+1) << "\t" << pl[i].name << "\t\t\t" << pl[i].avg_cap << 
			"\t" << pl[i].avg_opp << 
			"\t\t" << "(" << pl[i].plays.size() << " PA)" << endl;
	}

}




void output_player_strength () {
	// play_id, player, pitcher, game, atbat, // cap, opp
	
	
	cout << "play_id,player,player_name,pitcher,game,atbat,cap,opp,true,event,ppa" << endl;
			// iterate over each player individually
		for ( auto iter = players.begin(); iter != players.end(); ++iter ) {
			
			auto& player = iter->second;
			
			for ( int i = 0; i < player.plays.size(); ++i ) {
				
				auto play = plays[player.plays[i]];
			
				cout << play.play_id << "," << play.player << "," << play.player_name << "," 
					<< play.pitcher << "," << play.game << "," << play.atbat << ","
					<< player.cap[i] << "," << pitchers[play.pitcher].opp[player.pitcher_opp_id[i]] << ","
					<< player.true_measurement[i] << "," << play.event << "," << pitchers[play.pitcher].opp.size()
					
					<< endl;
			}
			
			
		}

}


int main() {
	
	// read in CSV
	
	
	
	
	if ( !OUTPUT_PLAY_STRENGTH ) {
		cout << "Reading in CSV" << endl;
	}
	
	read_csv();
	sort_plays();
	
	if ( !OUTPUT_PLAY_STRENGTH ) {
		cout << "Plays: " << plays.size() << endl;
	}
	
	// Prep
	
	setup_players();
	
	if ( !OUTPUT_PLAY_STRENGTH ) {
		cout << "Players: " << players.size() << endl;
		cout << "Pitchers: " << pitchers.size() << endl;
	}
	// now to solve
	
	if ( !OUTPUT_PLAY_STRENGTH ) {
		cout << "Iterating... " << flush;
	}
	for ( int i = 0; i < ITER; ++i ) {

		if ( !OUTPUT_PLAY_STRENGTH ) {
			cout << (i+1) << ' ' << flush;
		}
		iterate_players();
		iterate_pitchers();
	}
	if ( !OUTPUT_PLAY_STRENGTH ) {
		cout << endl;
	}
	
	// output results
	
	calculate_stats();
	if ( !OUTPUT_PLAY_STRENGTH ) {
		show_leaders();
	}
	
	// output CSV showing player strengths
	
	if ( OUTPUT_PLAY_STRENGTH ) {
		output_player_strength();
	}
	
	

}