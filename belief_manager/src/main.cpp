/* 
 * File:   main.cpp
 * Author: gmilliez
 *
 * Created on February 5, 2015, 2:49 PM
 */

#include "toaster_msgs/ToasterFactReader.h"
#include "toaster_msgs/ToasterObjectReader.h"
#include "toaster_msgs/FactList.h"
#include "toaster_msgs/Fact.h"
#include "toaster_msgs/AddFact.h"
#include "toaster_msgs/RemoveFact.h"

// factList for each monitored agent
static std::map<unsigned int, toaster_msgs::FactList> factListMap_;

// Agents with monitored belief
static std::map<std::string, unsigned int> agentsTracked_;


static unsigned int mainAgentId_ = 1;

// When adding a fact to an agent, the confidence may decrease as 
// the other's belief are suppositions based on observation

bool removeFactToAgent(unsigned int myFactId, unsigned int agentId) {
    factListMap_[agentId].factList.erase(factListMap_[agentId].factList.begin() + myFactId);
    return true;
}

bool removeFactToAgent(toaster_msgs::Fact myFact, unsigned int agentId) {
    bool removed = false;
    for (unsigned int i = 0; i < factListMap_[agentId].factList.size(); i++) {
        if ((factListMap_[agentId].factList[i].subjectName == myFact.subjectName) &&
                (factListMap_[agentId].factList[i].targetName == myFact.targetName) &&
                (factListMap_[agentId].factList[i].property == myFact.property)) {
            //we remove it:
            removeFactToAgent(i, agentId);
            removed = true;
        }
    }
    return removed;
}

bool removePropertyTypeToAgent(std::string propertyType, unsigned int agentId) {
    bool removed = false;
    for (unsigned int i = 0; i < factListMap_[agentId].factList.size(); i++) {
        if (factListMap_[agentId].factList[i].propertyType == propertyType) {
            //we remove it:
            removeFactToAgent(i, agentId);
            removed = true;
        }
    }
    return removed;
}

bool removeInternFactToAgent(unsigned int agentId) {
    bool removed = false;
    for (unsigned int i = 0; i < factListMap_[agentId].factList.size(); i++) {
        if ((factListMap_[agentId].factList[i].propertyType != "state") &&
                (factListMap_[agentId].factList[i].propertyType != "staticProperty")) {
            //we remove it:
            removeFactToAgent(i, agentId);
            removed = true;
        }
    }
    return removed;
}


// Extern fact are fact from request. They are managed by an external module.

bool addExternFactToAgent(toaster_msgs::Fact myFact, double confidenceDecrease, unsigned int agentId) {
    // We verify that this fact is not already there.
    for (unsigned int i = 0; i < factListMap_[agentId].factList.size(); i++) {
        if ((factListMap_[agentId].factList[i].subjectName == myFact.subjectName) &&
                (factListMap_[agentId].factList[i].property == myFact.property)) {
            // as it is the same fact, we remove the previous value:
            removeFactToAgent(i, agentId);
            printf("[BELIEF_MANAGER][WARNING] Fact added to agent %d was already in "
                    "current fact list: \n fact %s %s %s was removed to avoid double\n",
                    agentId, factListMap_[agentId].factList[i].subjectName.c_str(),
                    factListMap_[agentId].factList[i].property.c_str(),
                    factListMap_[agentId].factList[i].targetName.c_str());
        }
    }
    myFact.confidence *= confidenceDecrease;
    factListMap_[agentId].factList.push_back(myFact);
    return true;
}

bool addFactToAgent(toaster_msgs::Fact myFact, double confidenceDecrease, unsigned int agentId) {
    // We verify that this fact is not already there.
    for (unsigned int i = 0; i < factListMap_[agentId].factList.size(); i++) {
        if ((factListMap_[agentId].factList[i].subjectName == myFact.subjectName) &&
                (factListMap_[agentId].factList[i].targetName == myFact.targetName) &&
                (factListMap_[agentId].factList[i].property == myFact.property)) {
            // as it is the same fact, we remove the previous value:
            removeFactToAgent(i, agentId);
            printf("[BELIEF_MANAGER][WARNING] Fact added to agent %d was already in "
                    "current fact list: \n fact %s %s %s was removed to avoid double\n",
                    agentId, factListMap_[agentId].factList[i].subjectName.c_str(),
                    factListMap_[agentId].factList[i].property.c_str(),
                    factListMap_[agentId].factList[i].targetName.c_str());
        }
    }
    myFact.confidence *= confidenceDecrease;
    factListMap_[agentId].factList.push_back(myFact);
    return true;
}

bool addFact(toaster_msgs::AddFact::Request &req,
        toaster_msgs::AddFact::Response & res) {

    /**************************/
    /* World State management */
    /**************************/

    // Add safely the fact to main agent
    res.answer = addExternFactToAgent(req.fact, 1.0, mainAgentId_);


    // We update an agent belief state if he is in the room, has visibility on subject
    // and we don't care about the observability as we assess that the agent saw the action.

    /**********************************/
    /* Conceptual perspective taking: */
    /**********************************/


    // TODO: for each agent present and in same room
    for (std::map<std::string, unsigned int>::iterator it = agentsTracked_.begin(); it != agentsTracked_.end(); ++it) {
        for (unsigned int i_visibility = 0; i_visibility < factListMap_[mainAgentId_].factList.size(); i_visibility++) {


            if (factListMap_[mainAgentId_].factList[i_visibility].property == "IsVisible"
                    // Current agent
                    && factListMap_[mainAgentId_].factList[i_visibility].subjectName == it->first
                    // has visibility
                    && factListMap_[mainAgentId_].factList[i_visibility].targetName
                    // On current fact subject
                    == req.fact.subjectName) {

                addExternFactToAgent(req.fact, factListMap_[mainAgentId_].factList[i_visibility].doubleValue, it->second);
            }
        }
    }
    ROS_INFO("request: adding a new fact");
    ROS_INFO("sending back response: [%d]", (int) res.answer);
    return true;
}

bool removeFact(toaster_msgs::RemoveFact::Request &req,
        toaster_msgs::RemoveFact::Response & res) {


    /**************************/
    /* World State management */
    /**************************/

    // Remove safely the fact to main agent
    res.answer = removeFactToAgent(req.fact, mainAgentId_);


    /**********************************/
    /* Conceptual perspective taking: */
    /**********************************/


    // TODO: for each agent with visibility on subject
    for (std::map<std::string, unsigned int>::iterator it = agentsTracked_.begin(); it != agentsTracked_.end(); ++it) {
        for (unsigned int i_visibility = 0; i_visibility < factListMap_[mainAgentId_].factList.size(); i_visibility++) {


            if (factListMap_[mainAgentId_].factList[i_visibility].property == "IsVisible"
                    // Current agent
                    && factListMap_[mainAgentId_].factList[i_visibility].subjectName == it->first
                    // has visibility
                    && factListMap_[mainAgentId_].factList[i_visibility].targetName
                    // On current fact subject
                    == req.fact.subjectName) {
                removeFactToAgent(req.fact, it->second);
            }
        }
    }
    ROS_INFO("request: removing a fact");
    ROS_INFO("sending back response: [%d]", (int) res.answer);
    return true;

}

int main(int argc, char** argv) {
    // Set this in a ros service
    ros::init(argc, argv, "belief_manager");
    ros::NodeHandle node;


    //Data reading
    ToasterFactReader factRdSpark(node, "SPARK/factList");
    ToasterFactReader factRdSpar(node, "area_manager/factList");
    ToasterFactReader factRdAM(node, "agent_monitor/factList");
    //ToasterObjectReader objectRd(node);

    //Services
    ros::ServiceServer service = node.advertiseService("belief_manager/add_fact", addFact);
    ROS_INFO("Ready to add fact.");

    // Agent with monitored belief
    // TODO make a ros service
    agentsTracked_["HERAKLES_HUMAN1"] = 101;
    agentsTracked_["HERAKLES_HUMAN2"] = 102;

    static ros::Publisher fact_pub_main = node.advertise<toaster_msgs::FactList>("belief_manager/PR2_ROBOT/factList", 1000);
    static ros::Publisher fact_pub_human1 = node.advertise<toaster_msgs::FactList>("belief_manager/HERAKLES_HUMAN1/factList", 1000);
    static ros::Publisher fact_pub_human2 = node.advertise<toaster_msgs::FactList>("belief_manager/HERAKLES_HUMAN2/factList", 1000);

    toaster_msgs::FactList factList_msg;

    // Set this in a ros service?
    ros::Rate loop_rate(30);

    // Vector of Objects.
    //std::map<std::string, Object*> mapObject;


    /************************/
    /* Start of the Ros loop*/
    /************************/

    while (node.ok()) {


        /**************************/
        /* World State management */
        /**************************/

        // We remove intern fact for main agent
        removeInternFactToAgent(mainAgentId_);
        // First we feed the mainAgent belief state
        for (unsigned int i = 0; i < factRdSpar.lastMsgFact.factList.size(); i++) {
            addFactToAgent(factRdSpar.lastMsgFact.factList[i], 1.0, mainAgentId_);
        }

        for (unsigned int i = 0; i < factRdSpark.lastMsgFact.factList.size(); i++) {
            addFactToAgent(factRdSpark.lastMsgFact.factList[i], 1.0, mainAgentId_);
        }

        for (unsigned int i = 0; i < factRdAM.lastMsgFact.factList.size(); i++) {
            addFactToAgent(factRdAM.lastMsgFact.factList[i], 1.0, mainAgentId_);
        }


        /**********************************/
        /* Conceptual perspective taking: */
        /**********************************/

        // We remove facts that are visible before updating.
        for (std::map<std::string, unsigned int>::iterator it = agentsTracked_.begin(); it != agentsTracked_.end(); ++it) {
            for (unsigned int i = 0; i < factListMap_[it->second].factList.size(); i++) {
                // Update if:
                // 1) fact is observable
                if (factListMap_[mainAgentId_].factList[i].factObservability > 0.0) {

                    // 2) Agent has visibility on fact subject
                    for (unsigned int i_visibility = 0; i_visibility < factListMap_[mainAgentId_].factList.size(); i_visibility++) {


                        if (factListMap_[mainAgentId_].factList[i_visibility].property == "IsVisible"
                                // Current agent
                                && factListMap_[mainAgentId_].factList[i_visibility].subjectName == it->first
                                // has visibility
                                && factListMap_[mainAgentId_].factList[i_visibility].targetName
                                // On current fact subject
                                == factListMap_[it->second].factList[i].subjectName) {


                            /*printf("Agent %s has visibility on %s. We remove related fact to agent model before update"
                                    "\n fact added to %s: %s %s %s \n",
                                    it->first.c_str(),
                                    factListMap_[mainAgentId_].factList[i].subjectName.c_str(),
                                    it->first.c_str(),
                                    factListMap_[mainAgentId_].factList[i].subjectName.c_str(),
                                    factListMap_[mainAgentId_].factList[i].property.c_str(),
                                    factListMap_[mainAgentId_].factList[i].targetName.c_str());

                             */
                            removeFactToAgent(factListMap_[it->second].factList[i], it->second);
                        }
                    }
                }
            }


            // Agent observes new facts
            for (unsigned int i = 0; i < factListMap_[mainAgentId_].factList.size(); i++) {
                // Update if:
                // 1) fact is observable
                if (factListMap_[mainAgentId_].factList[i].factObservability > 0.0) {

                    // 2) Agent has visibility on fact subject
                    for (unsigned int i_visibility = 0; i_visibility < factListMap_[mainAgentId_].factList.size(); i_visibility++) {
                        if (factListMap_[mainAgentId_].factList[i_visibility].property == "IsVisible"
                                // Current agent
                                && factListMap_[mainAgentId_].factList[i_visibility].subjectName == it->first
                                // has visibility
                                && factListMap_[mainAgentId_].factList[i_visibility].targetName
                                // On current fact subject
                                == factListMap_[mainAgentId_].factList[i].subjectName) {


                            //printf("Agent %s has visibility on %s. We add related fact to agent model "
                            //        "\n fact added to %s: %s %s %s \n",
                            //        it->first.c_str(),
                            //        factListMap_[mainAgentId_].factList[i].subjectName.c_str(),
                            //        it->first.c_str(),
                            //        factListMap_[mainAgentId_].factList[i].subjectName.c_str(),
                            //        factListMap_[mainAgentId_].factList[i].property.c_str(),
                            //        factListMap_[mainAgentId_].factList[i].targetName.c_str());

                            
                             if ((factListMap_[mainAgentId_].factList[i].propertyType != "State") &&
                                    (factListMap_[mainAgentId_].factList[i].propertyType != "StaticProperty"))
                                addFactToAgent(factListMap_[mainAgentId_].factList[i], factListMap_[mainAgentId_].factList[i_visibility].doubleValue * factListMap_[mainAgentId_].factList[i].factObservability, it->second);
                            else
                                addExternFactToAgent(factListMap_[mainAgentId_].factList[i], factListMap_[mainAgentId_].factList[i_visibility].doubleValue * factListMap_[mainAgentId_].factList[i].factObservability, it->second);
                        }
                    }
                }

            }

        }

        //TODO: publish for each agent:
        fact_pub_main.publish(factListMap_[mainAgentId_]);
        fact_pub_human1.publish(factListMap_[101]);
        fact_pub_human2.publish(factListMap_[102]);

        ros::spinOnce();

        loop_rate.sleep();
    }
    return 0;
}

