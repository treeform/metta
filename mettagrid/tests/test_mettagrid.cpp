#include <gtest/gtest.h>

#include "actions/attack.hpp"
#include "actions/get_output.hpp"
#include "actions/put_recipe_items.hpp"
#include "event.hpp"
#include "grid.hpp"
#include "objects/agent.hpp"
#include "objects/constants.hpp"
#include "objects/converter.hpp"
#include "objects/wall.hpp"

// Pure C++ tests without any Python/pybind dependencies - we will test those with pytest
class MettaGridCppTest : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {}

  // Helper function to create test max_items_per_type map
  std::map<std::string, unsigned int> create_test_max_items_per_type() {
    std::map<std::string, unsigned int> max_items_per_type;
    max_items_per_type["heart"] = 50;
    max_items_per_type["ore.red"] = 50;
    max_items_per_type["ore.green"] = 50;
    max_items_per_type["ore.blue"] = 50;
    max_items_per_type["battery.red"] = 50;
    max_items_per_type["battery.green"] = 50;
    max_items_per_type["battery.blue"] = 50;
    max_items_per_type["laser"] = 50;
    return max_items_per_type;
  }

  // Helper function to create test rewards map
  std::map<std::string, float> create_test_rewards() {
    std::map<std::string, float> rewards;
    rewards["heart"] = 1.0f;
    rewards["ore.red"] = 0.125f;
    rewards["ore.green"] = 0.5f;
    rewards["ore.blue"] = 0.0f;
    rewards["battery.red"] = 0.0f;
    rewards["battery.green"] = 0.0f;
    rewards["battery.blue"] = 0.0f;
    rewards["laser"] = 0.0f;
    return rewards;
  }

  // Helper function to create test resource_reward_max map
  std::map<std::string, float> create_test_resource_reward_max() {
    std::map<std::string, float> resource_reward_max;
    resource_reward_max["heart"] = 10.0f;
    resource_reward_max["ore.red"] = 10.0f;
    resource_reward_max["ore.green"] = 10.0f;
    resource_reward_max["ore.blue"] = 10.0f;
    resource_reward_max["battery.red"] = 10.0f;
    resource_reward_max["battery.green"] = 10.0f;
    resource_reward_max["battery.blue"] = 10.0f;
    resource_reward_max["laser"] = 10.0f;
    return resource_reward_max;
  }
};

// ==================== Agent Tests ====================

TEST_F(MettaGridCppTest, AgentRewards) {
  auto max_items_per_type = create_test_max_items_per_type();
  auto rewards = create_test_rewards();
  auto resource_reward_max = create_test_resource_reward_max();

  std::unique_ptr<Agent> agent(
      new Agent(0, 0, 50, 100, 0.1f, max_items_per_type, rewards, resource_reward_max, "test_group", 1));

  // Test reward values
  EXPECT_FLOAT_EQ(agent->resource_rewards[InventoryItem::heart], 1.0f);
  EXPECT_FLOAT_EQ(agent->resource_rewards[InventoryItem::ore_red], 0.125f);
  EXPECT_FLOAT_EQ(agent->resource_rewards[InventoryItem::ore_green], 0.5f);
}

TEST_F(MettaGridCppTest, AgentInventoryUpdate) {
  auto max_items_per_type = create_test_max_items_per_type();
  auto rewards = create_test_rewards();
  auto resource_reward_max = create_test_resource_reward_max();

  std::unique_ptr<Agent> agent(
      new Agent(0, 0, 50, 100, 0.1f, max_items_per_type, rewards, resource_reward_max, "test_group", 1));

  float dummy_reward = 0.0f;
  agent->init(&dummy_reward);

  // Test adding items
  int delta = agent->update_inventory(InventoryItem::heart, 5);
  EXPECT_EQ(delta, 5);
  EXPECT_EQ(agent->inventory[InventoryItem::heart], 5);

  // Test removing items
  delta = agent->update_inventory(InventoryItem::heart, -2);
  EXPECT_EQ(delta, -2);
  EXPECT_EQ(agent->inventory[InventoryItem::heart], 3);

  // Test hitting zero
  delta = agent->update_inventory(InventoryItem::heart, -10);
  EXPECT_EQ(delta, -3);  // Should only remove what's available
  // check that the item is not in the inventory
  EXPECT_EQ(agent->inventory.find(InventoryItem::heart), agent->inventory.end());

  // Test hitting max_items_per_type limit
  agent->update_inventory(InventoryItem::heart, 30);
  delta = agent->update_inventory(InventoryItem::heart, 50);  // max_items_per_type is 50
  EXPECT_EQ(delta, 20);                                       // Should only add up to max_items_per_type
  EXPECT_EQ(agent->inventory[InventoryItem::heart], 50);
}

// ==================== Grid Tests ====================

TEST_F(MettaGridCppTest, GridCreation) {
  std::vector<Layer> layer_for_type_id;
  for (const auto& layer : ObjectLayers) {
    layer_for_type_id.push_back(layer.second);
  }

  Grid grid(10, 5, layer_for_type_id);

  EXPECT_EQ(grid.width, 10);
  EXPECT_EQ(grid.height, 5);
  EXPECT_EQ(grid.num_layers, GridLayer::GridLayerCount);
}

TEST_F(MettaGridCppTest, GridObjectManagement) {
  std::vector<Layer> layer_for_type_id;
  for (const auto& layer : ObjectLayers) {
    layer_for_type_id.push_back(layer.second);
  }

  Grid grid(10, 10, layer_for_type_id);

  // Create and add an agent
  auto max_items_per_type = create_test_max_items_per_type();
  auto rewards = create_test_rewards();
  auto resource_reward_max = create_test_resource_reward_max();
  Agent* agent = new Agent(2, 3, 50, 100, 0.1f, max_items_per_type, rewards, resource_reward_max, "test_group", 1);

  grid.add_object(agent);

  EXPECT_NE(agent->id, 0);  // Should have been assigned a valid ID
  EXPECT_EQ(agent->location.r, 2);
  EXPECT_EQ(agent->location.c, 3);

  // Verify we can retrieve the agent
  auto retrieved_agent = grid.object(agent->id);
  EXPECT_EQ(retrieved_agent, agent);

  // Verify it's at the expected location
  auto agent_at_location = grid.object_at(GridLocation(2, 3, GridLayer::Agent_Layer));
  EXPECT_EQ(agent_at_location, agent);
}

// ==================== Action Tests ====================

TEST_F(MettaGridCppTest, AttackAction) {
  std::vector<Layer> layer_for_type_id;
  for (const auto& layer : ObjectLayers) {
    layer_for_type_id.push_back(layer.second);
  }

  Grid grid(10, 10, layer_for_type_id);

  auto max_items_per_type = create_test_max_items_per_type();
  auto rewards = create_test_rewards();
  auto resource_reward_max = create_test_resource_reward_max();

  // Create attacker and target
  Agent* attacker = new Agent(2, 0, 50, 100, 0.1f, max_items_per_type, rewards, resource_reward_max, "red", 1);
  Agent* target = new Agent(0, 0, 50, 100, 0.1f, max_items_per_type, rewards, resource_reward_max, "blue", 2);

  float attacker_reward = 0.0f;
  float target_reward = 0.0f;
  attacker->init(&attacker_reward);
  target->init(&target_reward);

  grid.add_object(attacker);
  grid.add_object(target);

  // Give attacker a laser
  attacker->update_inventory(InventoryItem::laser, 1);
  EXPECT_EQ(attacker->inventory[InventoryItem::laser], 1);

  // Give target some items
  target->update_inventory(InventoryItem::heart, 2);
  target->update_inventory(InventoryItem::battery_red, 3);
  EXPECT_EQ(target->inventory[InventoryItem::heart], 2);
  EXPECT_EQ(target->inventory[InventoryItem::battery_red], 3);

  // Verify attacker orientation
  EXPECT_EQ(attacker->orientation, Orientation::Up);

  // Create attack action handler
  ActionConfig attack_cfg;
  Attack attack(attack_cfg);
  attack.init(&grid);

  // Perform attack (arg 5 targets directly in front)
  bool success = attack.handle_action(attacker->id, 5);
  EXPECT_TRUE(success);

  // Verify laser was consumed
  EXPECT_EQ(attacker->inventory[InventoryItem::laser], 0);

  // Verify target was frozen
  EXPECT_GT(target->frozen, 0);

  // Verify target's inventory was stolen
  EXPECT_EQ(target->inventory[InventoryItem::heart], 0);
  EXPECT_EQ(target->inventory[InventoryItem::battery_red], 0);
  EXPECT_EQ(attacker->inventory[InventoryItem::heart], 2);
  EXPECT_EQ(attacker->inventory[InventoryItem::battery_red], 3);
}

TEST_F(MettaGridCppTest, PutRecipeItems) {
  std::vector<Layer> layer_for_type_id;
  for (const auto& layer : ObjectLayers) {
    layer_for_type_id.push_back(layer.second);
  }

  Grid grid(10, 10, layer_for_type_id);

  auto max_items_per_type = create_test_max_items_per_type();
  auto rewards = create_test_rewards();
  auto resource_reward_max = create_test_resource_reward_max();

  Agent* agent = new Agent(1, 0, 50, 100, 0.1f, max_items_per_type, rewards, resource_reward_max, "red", 1);
  float agent_reward = 0.0f;
  agent->init(&agent_reward);

  grid.add_object(agent);

  // Create a generator that takes red ore and outputs batteries
  ObjectConfig generator_cfg;
  generator_cfg["input_ore.red"] = 1;
  generator_cfg["output_battery.red"] = 1;
  // Set the max_output to 0 so it won't consume things we put in it.
  generator_cfg["max_output"] = 0;
  generator_cfg["conversion_ticks"] = 1;
  generator_cfg["cooldown"] = 10;
  generator_cfg["initial_items"] = 0;

  EventManager event_manager;
  Converter* generator = new Converter(0, 0, generator_cfg, ObjectType::GeneratorT);
  grid.add_object(generator);
  generator->set_event_manager(&event_manager);

  // Give agent some items
  agent->update_inventory(InventoryItem::ore_red, 1);
  agent->update_inventory(InventoryItem::ore_blue, 1);

  // Create put_recipe_items action handler
  ActionConfig put_cfg;
  PutRecipeItems put(put_cfg);
  put.init(&grid);

  // Test putting matching items
  bool success = put.handle_action(agent->id, 0);
  EXPECT_TRUE(success);
  EXPECT_EQ(agent->inventory[InventoryItem::ore_red], 0);      // Red ore consumed
  EXPECT_EQ(agent->inventory[InventoryItem::ore_blue], 1);     // Blue ore unchanged
  EXPECT_EQ(generator->inventory[InventoryItem::ore_red], 1);  // Red ore added to generator

  // Test putting non-matching items
  success = put.handle_action(agent->id, 0);
  EXPECT_FALSE(success);                                        // Should fail since we only have blue ore left
  EXPECT_EQ(agent->inventory[InventoryItem::ore_blue], 1);      // Blue ore unchanged
  EXPECT_EQ(generator->inventory[InventoryItem::ore_blue], 0);  // No blue ore in generator
}

TEST_F(MettaGridCppTest, GetOutput) {
  std::vector<Layer> layer_for_type_id;
  for (const auto& layer : ObjectLayers) {
    layer_for_type_id.push_back(layer.second);
  }

  Grid grid(10, 10, layer_for_type_id);

  auto max_items_per_type = create_test_max_items_per_type();
  auto rewards = create_test_rewards();
  auto resource_reward_max = create_test_resource_reward_max();

  Agent* agent = new Agent(1, 0, 50, 100, 0.1f, max_items_per_type, rewards, resource_reward_max, "red", 1);
  float agent_reward = 0.0f;
  agent->init(&agent_reward);

  grid.add_object(agent);

  // Create a generator with initial output
  ObjectConfig generator_cfg;
  generator_cfg["input_ore.red"] = 1;
  generator_cfg["output_battery.red"] = 1;
  // Set the max_output to 0 so it won't consume things we put in it.
  generator_cfg["max_output"] = 1;
  generator_cfg["conversion_ticks"] = 1;
  generator_cfg["cooldown"] = 10;
  generator_cfg["initial_items"] = 1;

  EventManager event_manager;
  Converter* generator = new Converter(0, 0, generator_cfg, ObjectType::GeneratorT);
  grid.add_object(generator);
  generator->set_event_manager(&event_manager);

  // Give agent some items
  agent->update_inventory(InventoryItem::ore_red, 1);

  // Create get_output action handler
  ActionConfig get_cfg;
  GetOutput get(get_cfg);
  get.init(&grid);

  // Test getting output
  bool success = get.handle_action(agent->id, 0);
  EXPECT_TRUE(success);
  EXPECT_EQ(agent->inventory[InventoryItem::ore_red], 1);          // Still have red ore
  EXPECT_EQ(agent->inventory[InventoryItem::battery_red], 1);      // Also have a battery
  EXPECT_EQ(generator->inventory[InventoryItem::battery_red], 0);  // Generator gave away its battery
}

// ==================== Event System Tests ====================

TEST_F(MettaGridCppTest, EventManager) {
  std::vector<Layer> layer_for_type_id;
  for (const auto& layer : ObjectLayers) {
    layer_for_type_id.push_back(layer.second);
  }

  Grid grid(10, 10, layer_for_type_id);
  EventManager event_manager;

  // Test that event manager can be initialized
  // (This is a basic test - more complex event testing would require more setup)
  EXPECT_NO_THROW(event_manager.process_events(1));
}

// ==================== Object Type Tests ====================

TEST_F(MettaGridCppTest, ObjectTypes) {
  // Test that object type constants are properly defined
  EXPECT_NE(ObjectType::AgentT, ObjectType::WallT);
  EXPECT_NE(ObjectType::AgentT, ObjectType::GeneratorT);
  EXPECT_NE(ObjectType::WallT, ObjectType::GeneratorT);

  // Test that object layers are properly mapped
  EXPECT_TRUE(ObjectLayers.find(ObjectType::AgentT) != ObjectLayers.end());
  EXPECT_TRUE(ObjectLayers.find(ObjectType::WallT) != ObjectLayers.end());
  EXPECT_TRUE(ObjectLayers.find(ObjectType::GeneratorT) != ObjectLayers.end());
}

TEST_F(MettaGridCppTest, InventoryItems) {
  // Test that inventory item constants are properly defined
  EXPECT_NE(InventoryItem::heart, InventoryItem::battery_red);
  EXPECT_NE(InventoryItem::heart, InventoryItem::ore_red);
  EXPECT_NE(InventoryItem::battery_red, InventoryItem::ore_red);

  // Test that inventory item names exist
  EXPECT_FALSE(InventoryItemNames.empty());
  EXPECT_GT(InventoryItemNames.size(), 0);
}

// ==================== Wall/Block Tests ====================

TEST_F(MettaGridCppTest, WallCreation) {
  ObjectConfig wall_cfg;

  std::unique_ptr<Wall> wall(new Wall(2, 3, wall_cfg));

  ASSERT_NE(wall, nullptr);
  EXPECT_EQ(wall->location.r, 2);
  EXPECT_EQ(wall->location.c, 3);
}

// ==================== Converter Tests ====================

TEST_F(MettaGridCppTest, ConverterCreation) {
  ObjectConfig converter_cfg;
  converter_cfg["input_ore.red"] = 2;
  converter_cfg["output_battery.red"] = 1;
  converter_cfg["conversion_ticks"] = 5;
  converter_cfg["cooldown"] = 10;
  converter_cfg["initial_items"] = 0;

  std::unique_ptr<Converter> converter(new Converter(1, 2, converter_cfg, ObjectType::GeneratorT));

  ASSERT_NE(converter, nullptr);
  EXPECT_EQ(converter->location.r, 1);
  EXPECT_EQ(converter->location.c, 2);
}
